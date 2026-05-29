#pragma once
#include "RenderGraphResource.h"
#include <vulkan/vulkan.h>

namespace Kita {
	class VulkanContext;
	class VulkanRenderTarget;

	class RenderGraphContext
	{
	public:
		RenderGraphContext(VulkanContext& context, VkCommandBuffer commandBuffer, const std::vector<RenderGraphResource>& resources);
		VulkanContext& GetVulkanContext() const { return *m_Context; }
		VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; }

		VulkanRenderTarget& GetRenderTarget(RenderGraphResourceID id) const ;

	private:
		VulkanContext* m_Context = nullptr;
		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
		const std::vector<RenderGraphResource>* m_Resources = nullptr;
	};

}