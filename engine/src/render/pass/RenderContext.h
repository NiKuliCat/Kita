#pragma once
#include "render/VulkanRenderTargetView.h"
#include <vulkan/vulkan.h>

namespace Kita {

    class VulkanContext;
    class RenderPassContext
    {
    public:
        RenderPassContext(VulkanContext& context, VkCommandBuffer commandBuffer, const VulkanRenderTargetView& rtView);

        VulkanContext& GetContext() const;
        VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; }
        VulkanRenderTargetView& GetRenderTargetView() { return m_RenderTargetView; }
        const VulkanRenderTargetView& GetRenderTargetView() const { return m_RenderTargetView; }
        uint32_t GetFrameIndex() const;


    private:
        VulkanContext* m_Context = nullptr;
        VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
        VulkanRenderTargetView m_RenderTargetView{};
    };

}
