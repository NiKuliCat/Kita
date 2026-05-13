#pragma once
#include "VulkanBuffer.h"
#include "core/Core.h"
#include <vulkan/vulkan.h>

namespace Kita {


	class VulkanStorageBuffer
	{
	public:
		VulkanStorageBuffer() = default;
		VulkanStorageBuffer(VulkanContext& context, VkDeviceSize size, const std::string& name = "");
        ~VulkanStorageBuffer() = default;

        VulkanStorageBuffer(const VulkanStorageBuffer&) = delete;
        VulkanStorageBuffer& operator=(const VulkanStorageBuffer&) = delete;

        VulkanStorageBuffer(VulkanStorageBuffer&& other) noexcept;
        VulkanStorageBuffer& operator=(VulkanStorageBuffer&& other) noexcept;

        void Init(VulkanContext& context, VkDeviceSize size, const std::string& name = "");
        void Destroy();

        void SetData(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

        VkDescriptorBufferInfo GetDescriptorInfo() const;

        VkBuffer GetHandle() const { return m_Buffer ? m_Buffer->GetHandle() : VK_NULL_HANDLE; }
        VkDeviceSize GetSize() const { return m_Buffer ? m_Buffer->GetSize() : 0; }
        bool IsValid() const { return m_Buffer && m_Buffer->IsValid(); }
        const std::string& GetName() const { static const std::string s_Empty; return m_Buffer ? m_Buffer->GetName() : s_Empty; }

    private:
       Unique<VulkanBuffer> m_Buffer;
    };

}
