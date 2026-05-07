#include "kita_pch.h"
#include "VulkanIndexBuffer.h"
#include "VulkanContext.h"
#include "core/Log.h"
namespace Kita {

	VulkanIndexBuffer::VulkanIndexBuffer(VulkanContext& context, const CreateInfo& createInfo)
	{
		Init(context, createInfo);
	}

	void VulkanIndexBuffer::Init(VulkanContext& context, const CreateInfo& createInfo)
	{
		KITA_CORE_ASSERT(createInfo.Count > 0, "VulkanIndexBuffer count must be greater than zero");

		m_Count = createInfo.Count;
		m_Size = createInfo.Count * static_cast<uint32_t>(sizeof(uint32_t));
		m_Dynamic = createInfo.Dynamic;

		VulkanBuffer::CreateInfo bufferInfo{};
		bufferInfo.Name = createInfo.Name;
		bufferInfo.Size = m_Size;
		bufferInfo.InitialData = createInfo.InitialData;
		bufferInfo.InitialDataSize = createInfo.InitialData ? m_Size : 0;

		if (createInfo.Dynamic)
		{
			bufferInfo.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
			bufferInfo.MemoryProperties =
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			bufferInfo.PersistentMap = createInfo.PersistentMap;
		}
		else
		{
			bufferInfo.Usage =
				VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
				VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			bufferInfo.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			bufferInfo.PersistentMap = false;
		}

		m_Buffer.Init(context, bufferInfo);
	}

	void VulkanIndexBuffer::SetData(const uint32_t* data, uint32_t count, uint32_t firstIndex)
	{
		KITA_CORE_ASSERT(data, "VulkanIndexBuffer SetData data is null");
		KITA_CORE_ASSERT(count > 0, "VulkanIndexBuffer SetData count must be greater than zero");
		KITA_CORE_ASSERT(firstIndex + count <= m_Count, "VulkanIndexBuffer SetData range is out of bounds");

		const uint32_t sizeInBytes = count * static_cast<uint32_t>(sizeof(uint32_t));
		const uint32_t offsetInBytes = firstIndex * static_cast<uint32_t>(sizeof(uint32_t));

		m_Buffer.SetData(data, sizeInBytes, offsetInBytes);
	}

}