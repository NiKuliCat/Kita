#include "kita_pch.h"
#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "core/Log.h"
namespace Kita {

	namespace
	{
		void VKCheck(VkResult result, const char* message)
		{
			if (result != VK_SUCCESS)
			{
				KITA_CORE_ERROR("{0}, VkResult = {1}", message, static_cast<int32_t>(result));
				throw std::runtime_error(message);
			}
		}

		VkCommandBuffer BeginSingleTimeCommands(VulkanContext& context)
		{
			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandPool = context.GetCommandPool();
			allocInfo.commandBufferCount = 1;

			VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
			VKCheck(
				vkAllocateCommandBuffers(context.GetDevice(), &allocInfo, &commandBuffer),
				"Failed to allocate Vulkan single-time command buffer");

			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VKCheck(
				vkBeginCommandBuffer(commandBuffer, &beginInfo),
				"Failed to begin Vulkan single-time command buffer");

			return commandBuffer;
		}

		void EndSingleTimeCommands(VulkanContext& context, VkCommandBuffer commandBuffer)
		{
			VKCheck(
				vkEndCommandBuffer(commandBuffer),
				"Failed to end Vulkan single-time command buffer");

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			VKCheck(
				vkQueueSubmit(context.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE),
				"Failed to submit Vulkan single-time command buffer");

			VKCheck(
				vkQueueWaitIdle(context.GetGraphicsQueue()),
				"Failed to wait for Vulkan graphics queue idle");

			vkFreeCommandBuffers(context.GetDevice(), context.GetCommandPool(), 1, &commandBuffer);
		}
	}

	VulkanBuffer::VulkanBuffer(VulkanContext& context, const CreateInfo& createInfo)
	{
		Init(context, createInfo);
	}

	VulkanBuffer::~VulkanBuffer()
	{
		Destroy();
	}

	VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
		: m_Context(other.m_Context),
		m_Name(std::move(other.m_Name)),
		m_Buffer(other.m_Buffer),
		m_Memory(other.m_Memory),
		m_Size(other.m_Size),
		m_Usage(other.m_Usage),
		m_MemoryProperties(other.m_MemoryProperties),
		m_MappedData(other.m_MappedData),
		m_MappedOffset(other.m_MappedOffset),
		m_MappedSize(other.m_MappedSize),
		m_PersistentMap(other.m_PersistentMap)
	{
		other.m_Context = nullptr;
		other.m_Buffer = VK_NULL_HANDLE;
		other.m_Memory = VK_NULL_HANDLE;
		other.m_Size = 0;
		other.m_Usage = 0;
		other.m_MemoryProperties = 0;
		other.m_MappedData = nullptr;
		other.m_MappedOffset = 0;
		other.m_MappedSize = 0;
		other.m_PersistentMap = false;
	}

	VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept
	{
		if (this == &other)
			return *this;

		Destroy();

		m_Context = other.m_Context;
		m_Name = std::move(other.m_Name);
		m_Buffer = other.m_Buffer;
		m_Memory = other.m_Memory;
		m_Size = other.m_Size;
		m_Usage = other.m_Usage;
		m_MemoryProperties = other.m_MemoryProperties;
		m_MappedData = other.m_MappedData;
		m_MappedOffset = other.m_MappedOffset;
		m_MappedSize = other.m_MappedSize;
		m_PersistentMap = other.m_PersistentMap;

		other.m_Context = nullptr;
		other.m_Buffer = VK_NULL_HANDLE;
		other.m_Memory = VK_NULL_HANDLE;
		other.m_Size = 0;
		other.m_Usage = 0;
		other.m_MemoryProperties = 0;
		other.m_MappedData = nullptr;
		other.m_MappedOffset = 0;
		other.m_MappedSize = 0;
		other.m_PersistentMap = false;

		return *this;
	}

	void VulkanBuffer::Init(VulkanContext& context, const CreateInfo& createInfo)
	{
		Destroy();

		m_Context = &context;
		m_Name = createInfo.Name;
		CreateBuffer(createInfo);
	}

	void VulkanBuffer::Destroy()
	{
		if (!m_Context)
			return;

		VkDevice device = m_Context->GetDevice();
		if (device == VK_NULL_HANDLE)
		{
			m_Context = nullptr;
			return;
		}

		if (m_MappedData)
		{
			vkUnmapMemory(device, m_Memory);
			m_MappedData = nullptr;
			m_MappedOffset = 0;
			m_MappedSize = 0;
		}

		if (m_Buffer != VK_NULL_HANDLE)
		{
			vkDestroyBuffer(device, m_Buffer, nullptr);
			m_Buffer = VK_NULL_HANDLE;
		}

		if (m_Memory != VK_NULL_HANDLE)
		{
			vkFreeMemory(device, m_Memory, nullptr);
			m_Memory = VK_NULL_HANDLE;
		}

		m_Size = 0;
		m_Usage = 0;
		m_MemoryProperties = 0;
		m_PersistentMap = false;
		m_Name.clear();
		m_Context = nullptr;
	}

	void* VulkanBuffer::Map(VkDeviceSize size, VkDeviceSize offset)
	{
		KITA_CORE_ASSERT(m_Context, "VulkanBuffer context is null");
		KITA_CORE_ASSERT(IsHostVisible(), "Cannot map non-host-visible Vulkan buffer");
		KITA_CORE_ASSERT(offset <= m_Size, "VulkanBuffer map offset is out of range");

		if (size == VK_WHOLE_SIZE)
			size = m_Size - offset;

		KITA_CORE_ASSERT(offset + size <= m_Size, "VulkanBuffer map range is out of bounds");

		if (m_MappedData)
		{
			KITA_CORE_ASSERT(
				offset >= m_MappedOffset && (offset + size) <= (m_MappedOffset + m_MappedSize),
				"Requested VulkanBuffer map range is outside the current mapped range");
			return GetMappedPointer(offset);
		}

		void* mapped = nullptr;
		VKCheck(
			vkMapMemory(m_Context->GetDevice(), m_Memory, offset, size, 0, &mapped),
			"Failed to map Vulkan buffer memory");

		m_MappedData = mapped;
		m_MappedOffset = offset;
		m_MappedSize = size;
		return m_MappedData;
	}

	void VulkanBuffer::Unmap()
	{
		if (!m_Context || !m_MappedData)
			return;

		vkUnmapMemory(m_Context->GetDevice(), m_Memory);
		m_MappedData = nullptr;
		m_MappedOffset = 0;
		m_MappedSize = 0;
	}

	void VulkanBuffer::Flush(VkDeviceSize size, VkDeviceSize offset) const
	{
		KITA_CORE_ASSERT(m_Context, "VulkanBuffer context is null");
		KITA_CORE_ASSERT(IsHostVisible(), "Cannot flush non-host-visible Vulkan buffer memory");

		if (IsHostCoherent())
			return;

		if (size == VK_WHOLE_SIZE)
			size = m_Size - offset;

		KITA_CORE_ASSERT(offset + size <= m_Size, "VulkanBuffer flush range is out of bounds");

		VkMappedMemoryRange range{};
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = m_Memory;
		range.offset = offset;
		range.size = size;

		VKCheck(
			vkFlushMappedMemoryRanges(m_Context->GetDevice(), 1, &range),
			"Failed to flush Vulkan buffer memory");
	}

	void VulkanBuffer::Invalidate(VkDeviceSize size, VkDeviceSize offset) const
	{
		KITA_CORE_ASSERT(m_Context, "VulkanBuffer context is null");
		KITA_CORE_ASSERT(IsHostVisible(), "Cannot invalidate non-host-visible Vulkan buffer memory");

		if (IsHostCoherent())
			return;

		if (size == VK_WHOLE_SIZE)
			size = m_Size - offset;

		KITA_CORE_ASSERT(offset + size <= m_Size, "VulkanBuffer invalidate range is out of bounds");

		VkMappedMemoryRange range{};
		range.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE;
		range.memory = m_Memory;
		range.offset = offset;
		range.size = size;

		VKCheck(
			vkInvalidateMappedMemoryRanges(m_Context->GetDevice(), 1, &range),
			"Failed to invalidate Vulkan buffer memory");
	}

	void VulkanBuffer::SetData(const void* data, VkDeviceSize size, VkDeviceSize offset)
	{
		KITA_CORE_ASSERT(data, "VulkanBuffer SetData data is null");
		KITA_CORE_ASSERT(size > 0, "VulkanBuffer SetData size must be greater than zero");
		KITA_CORE_ASSERT(offset + size <= m_Size, "VulkanBuffer SetData range is out of bounds");

		if (IsHostVisible())
		{
			const bool wasMapped = (m_MappedData != nullptr);

			if (!wasMapped)
			{
				if (m_PersistentMap)
					Map(m_Size, 0);
				else
					Map(size, offset);
			}

			void* dst = GetMappedPointer(offset);
			std::memcpy(dst, data, static_cast<size_t>(size));

			if (!IsHostCoherent())
				Flush(size, offset);

			if (!wasMapped && !m_PersistentMap)
				Unmap();

			return;
		}

		UploadWithStaging(data, size, offset);
	}

	void VulkanBuffer::CopyFrom(const VulkanBuffer& src, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset)
	{
		KITA_CORE_ASSERT(m_Context, "VulkanBuffer context is null");
		KITA_CORE_ASSERT(src.m_Context, "Source VulkanBuffer context is null");
		KITA_CORE_ASSERT(m_Context == src.m_Context, "Cannot copy between buffers from different Vulkan contexts");
		KITA_CORE_ASSERT((src.m_Usage & VK_BUFFER_USAGE_TRANSFER_SRC_BIT) != 0, "Source VulkanBuffer must have TRANSFER_SRC usage");
		KITA_CORE_ASSERT((m_Usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) != 0, "Destination VulkanBuffer must have TRANSFER_DST usage");
		KITA_CORE_ASSERT(srcOffset + size <= src.m_Size, "Source VulkanBuffer copy range is out of bounds");
		KITA_CORE_ASSERT(dstOffset + size <= m_Size, "Destination VulkanBuffer copy range is out of bounds");

		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(*m_Context);

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = srcOffset;
		copyRegion.dstOffset = dstOffset;
		copyRegion.size = size;

		vkCmdCopyBuffer(commandBuffer, src.m_Buffer, m_Buffer, 1, &copyRegion);

		EndSingleTimeCommands(*m_Context, commandBuffer);
	}

	bool VulkanBuffer::IsHostVisible() const
	{
		return (m_MemoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0;
	}

	bool VulkanBuffer::IsHostCoherent() const
	{
		return (m_MemoryProperties & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT) != 0;
	}

	VulkanContext& VulkanBuffer::GetContext() const
	{
		KITA_CORE_ASSERT(m_Context, "VulkanBuffer context is null");
		return *m_Context;
	}

	uint32_t VulkanBuffer::FindMemoryType(VulkanContext& context, uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		VkPhysicalDeviceMemoryProperties memProperties{};
		vkGetPhysicalDeviceMemoryProperties(context.GetPhysicalDevice(), &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; ++i)
		{
			const bool typeMatches = (typeFilter & (1u << i)) != 0;
			const bool propsMatch = (memProperties.memoryTypes[i].propertyFlags & properties) == properties;
			if (typeMatches && propsMatch)
				return i;
		}

		KITA_CORE_ASSERT(false, "Failed to find suitable Vulkan memory type");
		return 0;
	}

	void VulkanBuffer::CreateBuffer(const CreateInfo& createInfo)
	{
		KITA_CORE_ASSERT(m_Context, "VulkanBuffer context is null");
		KITA_CORE_ASSERT(createInfo.Size > 0, "VulkanBuffer size must be greater than zero");
		KITA_CORE_ASSERT(createInfo.Usage != 0, "VulkanBuffer usage must not be zero");

		if (createInfo.PersistentMap)
		{
			KITA_CORE_ASSERT(
				(createInfo.MemoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) != 0,
				"Persistent-mapped VulkanBuffer must use HOST_VISIBLE memory");
		}

		m_Size = createInfo.Size;
		m_Usage = createInfo.Usage;
		m_MemoryProperties = createInfo.MemoryProperties;
		m_PersistentMap = createInfo.PersistentMap;

		VkBufferCreateInfo bufferInfo{};
		bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		bufferInfo.size = createInfo.Size;
		bufferInfo.usage = createInfo.Usage;
		bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VKCheck(
			vkCreateBuffer(m_Context->GetDevice(), &bufferInfo, nullptr, &m_Buffer),
			"Failed to create Vulkan buffer");

		VkMemoryRequirements memRequirements{};
		vkGetBufferMemoryRequirements(m_Context->GetDevice(), m_Buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(*m_Context, memRequirements.memoryTypeBits, createInfo.MemoryProperties);

		VKCheck(
			vkAllocateMemory(m_Context->GetDevice(), &allocInfo, nullptr, &m_Memory),
			"Failed to allocate Vulkan buffer memory");

		VKCheck(
			vkBindBufferMemory(m_Context->GetDevice(), m_Buffer, m_Memory, 0),
			"Failed to bind Vulkan buffer memory");

		if (m_PersistentMap)
			Map(m_Size, 0);

		if (createInfo.InitialData)
		{
			VkDeviceSize initialSize = createInfo.InitialDataSize == 0 ? createInfo.Size : createInfo.InitialDataSize;
			KITA_CORE_ASSERT(initialSize <= createInfo.Size, "Initial VulkanBuffer data size exceeds buffer size");
			SetData(createInfo.InitialData, initialSize, 0);
		}
	}

	void VulkanBuffer::UploadWithStaging(const void* data, VkDeviceSize size, VkDeviceSize offset)
	{
		KITA_CORE_ASSERT(
			(m_Usage & VK_BUFFER_USAGE_TRANSFER_DST_BIT) != 0,
			"Non-host-visible VulkanBuffer requires TRANSFER_DST usage to upload data via staging");

		CreateInfo stagingInfo{};
		stagingInfo.Name = m_Name.empty() ? "StagingBuffer" : (m_Name + "_Staging");
		stagingInfo.Size = size;
		stagingInfo.Usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
		stagingInfo.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
		stagingInfo.InitialData = data;
		stagingInfo.InitialDataSize = size;
		stagingInfo.PersistentMap = false;

		VulkanBuffer staging(*m_Context, stagingInfo);
		CopyFrom(staging, size, 0, offset);
	}

	void* VulkanBuffer::GetMappedPointer(VkDeviceSize offset) const
	{
		KITA_CORE_ASSERT(m_MappedData, "VulkanBuffer is not mapped");
		KITA_CORE_ASSERT(offset >= m_MappedOffset, "VulkanBuffer mapped pointer offset is invalid");
		KITA_CORE_ASSERT(offset <= (m_MappedOffset + m_MappedSize), "VulkanBuffer mapped pointer offset is out of bounds");

		uint8_t* base = static_cast<uint8_t*>(m_MappedData);
		return base + static_cast<size_t>(offset - m_MappedOffset);
	}

}