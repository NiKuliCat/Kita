#include "kita_pch.h"
#include "VulkanStorageBuffer.h"
#include "core/Core.h"
#include "core/Log.h"


namespace Kita {
	VulkanStorageBuffer::VulkanStorageBuffer(VulkanContext& context, VkDeviceSize size, const std::string& name)
	{
		Init(context, size, name);
	}
	VulkanStorageBuffer::VulkanStorageBuffer(VulkanStorageBuffer&& other) noexcept
		:m_Buffer(std::move(other.m_Buffer))
	{
	}
	VulkanStorageBuffer& VulkanStorageBuffer::operator=(VulkanStorageBuffer&& other) noexcept
	{
		if (this != &other)
			m_Buffer = std::move(other.m_Buffer);
		return *this;
	}
	void VulkanStorageBuffer::Init(VulkanContext& context, VkDeviceSize size, const std::string& name)
	{
		KITA_CORE_ASSERT(size > 0, "VulkanStorageBuffer: size must be > 0");

		VulkanBuffer::CreateInfo createInfo{};
		createInfo.Name = name.empty() ? "StorageBuffer" : name;
		createInfo.Size = size;
		createInfo.Usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
		createInfo.MemoryProperties =
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		m_Buffer = CreateUnique<VulkanBuffer>(context, createInfo);
	}
	void VulkanStorageBuffer::Destroy()
	{
		m_Buffer.reset();
	}
	void VulkanStorageBuffer::SetData(const void* data, VkDeviceSize size, VkDeviceSize offset)
	{
		KITA_CORE_ASSERT(m_Buffer && m_Buffer->IsValid(), "VulkanStorageBuffer: not initialized");
		KITA_CORE_ASSERT(offset + size <= m_Buffer->GetSize(), "VulkanStorageBuffer: SetData out of bounds");
		m_Buffer->SetData(data, size, offset);

	}
	VkDescriptorBufferInfo VulkanStorageBuffer::GetDescriptorInfo() const
	{
		KITA_CORE_ASSERT(m_Buffer && m_Buffer->IsValid(), "VulkanStorageBuffer: not initialized");

		VkDescriptorBufferInfo info{};
		info.buffer = m_Buffer->GetHandle();
		info.offset = 0;
		info.range = m_Buffer->GetSize();
		return info;
	}
}
