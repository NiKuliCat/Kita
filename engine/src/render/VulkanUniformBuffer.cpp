#include "kita_pch.h"
#include "VulkanUniformBuffer.h"

#include "VulkanBuffer.h"
#include "VulkanContext.h"
#include "core/Log.h"

namespace Kita {

    VulkanUniformBuffer::VulkanUniformBuffer(VulkanContext& context, VkDeviceSize size, const std::string& name)
    {
        Init(context, size, name);
    }

    VulkanUniformBuffer::~VulkanUniformBuffer() = default;

    VulkanUniformBuffer::VulkanUniformBuffer(VulkanUniformBuffer&& other) noexcept
        : m_Buffer(std::move(other.m_Buffer))
    {
    }

    VulkanUniformBuffer& VulkanUniformBuffer::operator=(VulkanUniformBuffer&& other) noexcept
    {
        if (this != &other)
            m_Buffer = std::move(other.m_Buffer);
        return *this;
    }

    void VulkanUniformBuffer::Init(VulkanContext& context, VkDeviceSize size, const std::string& name)
    {
        KITA_CORE_ASSERT(size > 0, "VulkanUniformBuffer: size must be > 0");

        VulkanBuffer::CreateInfo createInfo{};
        createInfo.Name             = name.empty() ? "UniformBuffer" : name;
        createInfo.Size             = size;
        createInfo.Usage            = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        createInfo.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        m_Buffer = std::make_unique<VulkanBuffer>(context, createInfo);
    }

    void VulkanUniformBuffer::Destroy()
    {
        m_Buffer.reset();
    }

    void VulkanUniformBuffer::SetData(const void* data, VkDeviceSize size, VkDeviceSize offset)
    {
        KITA_CORE_ASSERT(m_Buffer && m_Buffer->IsValid(), "VulkanUniformBuffer: not initialized");
        KITA_CORE_ASSERT(offset + size <= m_Buffer->GetSize(), "VulkanUniformBuffer: SetData out of bounds");
        m_Buffer->SetData(data, size, offset);
    }

    VkDescriptorBufferInfo VulkanUniformBuffer::GetDescriptorInfo() const
    {
        KITA_CORE_ASSERT(m_Buffer && m_Buffer->IsValid(), "VulkanUniformBuffer: not initialized");

        VkDescriptorBufferInfo info{};
        info.buffer = m_Buffer->GetHandle();
        info.offset = 0;
        info.range  = m_Buffer->GetSize();
        return info;
    }

    VkBuffer VulkanUniformBuffer::GetHandle() const
    {
        return m_Buffer ? m_Buffer->GetHandle() : VK_NULL_HANDLE;
    }

    VkDeviceSize VulkanUniformBuffer::GetSize() const
    {
        return m_Buffer ? m_Buffer->GetSize() : 0;
    }

    bool VulkanUniformBuffer::IsValid() const
    {
        return m_Buffer && m_Buffer->IsValid();
    }

    const std::string& VulkanUniformBuffer::GetName() const
    {
        static const std::string s_Empty;
        return m_Buffer ? m_Buffer->GetName() : s_Empty;
    }

}
