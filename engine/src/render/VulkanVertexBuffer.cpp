#include "kita_pch.h"
#include "VulkanVertexBuffer.h"
#include "VulkanContext.h"
#include "core/Log.h"
namespace Kita {

	VulkanVertexBuffer::VulkanVertexBuffer(VulkanContext& context, const CreateInfo& createInfo)
	{
		Init(context, createInfo);
	}

	void VulkanVertexBuffer::Init(VulkanContext& context, const CreateInfo& createInfo)
	{
		KITA_CORE_ASSERT(createInfo.Size > 0, "VulkanVertexBuffer size must be greater than zero");

		m_Layout = createInfo.Layout;
		m_Size = createInfo.Size;
		m_Dynamic = createInfo.Dynamic;

		VulkanBuffer::CreateInfo bufferInfo{};
		bufferInfo.Name = createInfo.Name;
		bufferInfo.Size = createInfo.Size;
		bufferInfo.InitialData = createInfo.InitialData;
		bufferInfo.InitialDataSize =
			(createInfo.InitialDataSize == 0 && createInfo.InitialData != nullptr)
			? createInfo.Size
			: createInfo.InitialDataSize;

		if (createInfo.Dynamic)
		{
			bufferInfo.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			bufferInfo.MemoryProperties =
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			bufferInfo.PersistentMap = createInfo.PersistentMap;
		}
		else
		{
			bufferInfo.Usage =
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
				VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			bufferInfo.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			bufferInfo.PersistentMap = false;
		}

		m_Buffer.Init(context, bufferInfo);
	}

	void VulkanVertexBuffer::SetData(const void* data, uint32_t size, uint32_t offset)
	{
		KITA_CORE_ASSERT(data, "VulkanVertexBuffer SetData data is null");
		KITA_CORE_ASSERT(size > 0, "VulkanVertexBuffer SetData size must be greater than zero");
		KITA_CORE_ASSERT(offset + size <= m_Size, "VulkanVertexBuffer SetData range is out of bounds");

		m_Buffer.SetData(data, size, offset);
	}

}