#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
namespace Kita {

	class VulkanContext;
	class VulkanGraphicsPipeline;
	class VulkanGeometry;

	class VulkanRenderCommand
	{
	public:
		static void BeginSwapchainRendering(VulkanContext& context, const glm::vec4& clearColor);
		static void EndRendering(VulkanContext& context);

		static void SetViewport(VkCommandBuffer commandBuffer, uint32_t width, uint32_t height);
		static void SetScissor(VkCommandBuffer commandBuffer, uint32_t width, uint32_t height);

		static void BindPipeline(VkCommandBuffer commandBuffer, const VulkanGraphicsPipeline& pipeline);
		static void BindGeometry(VkCommandBuffer commandBuffer, const VulkanGeometry& geometry);

		static void DrawGeometry(VkCommandBuffer commandBuffer, const VulkanGeometry& geometry);
	};

}