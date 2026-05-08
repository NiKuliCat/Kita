#pragma once

#include "VulkanBuffer.h"
#include <vulkan/vulkan.h>
#include <memory>

namespace Kita {

    class VulkanContext;

    // ========================================================================
    // VulkanUniformBuffer — VkBuffer 的 UBO 特化封装
    //
    // 内部持有 VulkanBuffer（VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT +
    // VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT）
    //
    // 典型用法：
    //   VulkanUniformBuffer cameraUBO;
    //   cameraUBO.Init(context, sizeof(CameraData), "CameraUBO");
    //   cameraUBO.SetData(&cameraData, sizeof(CameraData));
    //   VkDescriptorBufferInfo info = cameraUBO.GetDescriptorInfo();
    // ========================================================================
    class VulkanUniformBuffer
    {
    public:
        VulkanUniformBuffer() = default;
        VulkanUniformBuffer(VulkanContext& context, VkDeviceSize size, const std::string& name = "");
        ~VulkanUniformBuffer();

        VulkanUniformBuffer(const VulkanUniformBuffer&) = delete;
        VulkanUniformBuffer& operator=(const VulkanUniformBuffer&) = delete;

        VulkanUniformBuffer(VulkanUniformBuffer&& other) noexcept;
        VulkanUniformBuffer& operator=(VulkanUniformBuffer&& other) noexcept;

        void Init(VulkanContext& context, VkDeviceSize size, const std::string& name = "");
        void Destroy();

        // 更新 UBO 数据（内部使用 staging buffer，可在 render pass 外调用）
        void SetData(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);

        // 获取 DescriptorSet 写入所需的 buffer info
        VkDescriptorBufferInfo GetDescriptorInfo() const;

        VkBuffer     GetHandle() const;
        VkDeviceSize GetSize()   const;
        bool         IsValid()   const;
        const std::string& GetName() const;

    private:
        std::unique_ptr<VulkanBuffer> m_Buffer;
    };

}
