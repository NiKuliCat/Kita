#pragma once

#include <vulkan/vulkan.h>
#include <vector>
#include <string>

namespace Kita {

    class VulkanContext;
    class VulkanUniformBuffer;

    // ========================================================================
    // VulkanDescriptorSet — 描述符集生命周期管理
    //
    // 封装 VkDescriptorSetLayout → VkDescriptorPool → VkDescriptorSet 的
    // 完整创建流程，提供 Write* / Bind 便捷方法。
    //
    // 典型用法：
    //   VulkanDescriptorSet::CreateInfo info;
    //   info.Bindings = {
    //       { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT },
    //       { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
    //   };
    //
    //   VulkanDescriptorSet descSet;
    //   descSet.Init(context, info);
    //   descSet.WriteUniformBuffer(0, cameraUBO);
    //   descSet.Bind(cmd, pipelineLayout, 0);
    // ========================================================================
    class VulkanDescriptorSet
    {
    public:
        struct Binding
        {
            uint32_t            binding   = 0;
            VkDescriptorType    type      = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uint32_t            count     = 1;
            VkShaderStageFlags  stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
        };

        struct CreateInfo
        {
            std::string          Name;
            std::vector<Binding> Bindings;
        };

    public:
        VulkanDescriptorSet() = default;
        VulkanDescriptorSet(VulkanContext& context, const CreateInfo& createInfo);
        ~VulkanDescriptorSet();

        VulkanDescriptorSet(const VulkanDescriptorSet&) = delete;
        VulkanDescriptorSet& operator=(const VulkanDescriptorSet&) = delete;

        VulkanDescriptorSet(VulkanDescriptorSet&& other) noexcept;
        VulkanDescriptorSet& operator=(VulkanDescriptorSet&& other) noexcept;

        void Init(VulkanContext& context, const CreateInfo& createInfo);
        void Destroy();

        // ---- 写入绑定 ----
        // 每次调用立即更新 descriptor set（vkUpdateDescriptorSets）

        void WriteUniformBuffer(uint32_t binding, VkBuffer buffer,
                                VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);
        void WriteUniformBuffer(uint32_t binding, const VulkanUniformBuffer& ubo);

        void WriteStorageBuffer(uint32_t binding, VkBuffer buffer,
                                VkDeviceSize offset = 0, VkDeviceSize range = VK_WHOLE_SIZE);

        void WriteImageSampler(uint32_t binding, VkImageView view, VkSampler sampler,
                               VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        void WriteImageSampler(uint32_t binding, const VkDescriptorImageInfo& info);

        // ---- 绑定到命令缓冲区 ----
        void Bind(VkCommandBuffer cmd, VkPipelineLayout layout,
                  uint32_t firstSet = 0,
                  const std::vector<uint32_t>& dynamicOffsets = {}) const;

        // ---- 访问器 ----
        VkDescriptorSetLayout GetLayout()  const { return m_Layout; }
        VkDescriptorSet       GetHandle()  const { return m_Set; }
        bool                  IsValid()    const { return m_Set != VK_NULL_HANDLE; }
        const std::string&    GetName()    const { return m_Name; }
        const CreateInfo&     GetInfo()    const { return m_Info; }

    private:
        void CreateLayout();
        void CreatePool();
        void AllocateSet();

    private:
        VulkanContext* m_Context = nullptr;
        std::string    m_Name;

        VkDescriptorSetLayout m_Layout = VK_NULL_HANDLE;
        VkDescriptorPool      m_Pool   = VK_NULL_HANDLE;
        VkDescriptorSet       m_Set    = VK_NULL_HANDLE;

        CreateInfo m_Info;
    };

}
