#pragma once
#include <vulkan/vulkan.h>

namespace Kita {

    class VulkanContext;
    class VulkanRenderTarget;

    class RenderPassContext
    {
    public:
        RenderPassContext(VulkanContext& context, VkCommandBuffer commandBuffer, VulkanRenderTarget& rt);

        VulkanContext& GetContext() const;
        VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; }
        VulkanRenderTarget& GetRenderTarget() const;
        uint32_t GetFrameIndex() const;


    private:
        VulkanContext* m_Context = nullptr;
        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
        VulkanRenderTarget* m_RenderTarget = nullptr;
    };

}