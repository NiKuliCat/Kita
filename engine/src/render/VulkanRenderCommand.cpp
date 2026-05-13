#include "kita_pch.h"
#include "VulkanRenderCommand.h"

#include "VulkanContext.h"
#include "VulkanGeometry.h"
#include "VulkanGraphicsPipeline.h"

#include "core/Log.h"

namespace Kita {

	namespace
	{
		void TransitionSwapchainImageForColorAttachment(
			VkCommandBuffer commandBuffer,
			VkImage image,
			VkImageLayout oldLayout)
		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = oldLayout;
			barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
				? VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
				: 0;
			barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

			VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			if (oldLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR || oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
				srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

			vkCmdPipelineBarrier(
				commandBuffer,
				srcStage,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier);
		}

		void TransitionSwapchainImageForPresent(
			VkCommandBuffer commandBuffer,
			VkImage image)
		{
			VkImageMemoryBarrier barrier{};
			barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			barrier.image = image;
			barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			barrier.subresourceRange.baseMipLevel = 0;
			barrier.subresourceRange.levelCount = 1;
			barrier.subresourceRange.baseArrayLayer = 0;
			barrier.subresourceRange.layerCount = 1;
			barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			barrier.dstAccessMask = 0;

			vkCmdPipelineBarrier(
				commandBuffer,
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
				VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
				0,
				0, nullptr,
				0, nullptr,
				1, &barrier);
		}
	}

	void VulkanRenderCommand::BeginSwapchainRendering(VulkanContext& context, const glm::vec4& clearColor)
	{
		VkCommandBuffer commandBuffer = context.GetCurrentCommandBuffer();
		KITA_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE, "VulkanRenderCommand command buffer is null");

		const uint32_t imageIndex = context.GetCurrentImageIndex();
		const auto& swapchainImages = context.GetSwapchainImages();
		const auto& swapchainImageViews = context.GetSwapchainImageViews();

		KITA_CORE_ASSERT(imageIndex < swapchainImages.size(), "Swapchain image index out of range");
		KITA_CORE_ASSERT(imageIndex < swapchainImageViews.size(), "Swapchain image view index out of range");

		VkImage image = swapchainImages[imageIndex];
		VkImageView imageView = swapchainImageViews[imageIndex];
		VkImageLayout oldLayout = context.GetSwapchainImageLayout(imageIndex);

		TransitionSwapchainImageForColorAttachment(commandBuffer, image, oldLayout);
		context.SetSwapchainImageLayout(imageIndex, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		VkClearValue clearValue{};
		clearValue.color = { { clearColor.r, clearColor.g, clearColor.b, clearColor.a } };

		VkRenderingAttachmentInfo colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		colorAttachment.imageView = imageView;
		colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		colorAttachment.resolveMode = VK_RESOLVE_MODE_NONE;
		colorAttachment.resolveImageView = VK_NULL_HANDLE;
		colorAttachment.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.clearValue = clearValue;

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.offset = { 0, 0 };
		renderingInfo.renderArea.extent = context.GetSwapchainExtent();
		renderingInfo.layerCount = 1;
		renderingInfo.viewMask = 0;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachments = &colorAttachment;
		renderingInfo.pDepthAttachment = nullptr;
		renderingInfo.pStencilAttachment = nullptr;

		vkCmdBeginRendering(commandBuffer, &renderingInfo);

		SetViewport(commandBuffer, context.GetSwapchainExtent().width, context.GetSwapchainExtent().height);
		SetScissor(commandBuffer, context.GetSwapchainExtent().width, context.GetSwapchainExtent().height);
	}

	void VulkanRenderCommand::EndRendering(VulkanContext& context)
	{
		VkCommandBuffer commandBuffer = context.GetCurrentCommandBuffer();
		KITA_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE, "VulkanRenderCommand command buffer is null");

		vkCmdEndRendering(commandBuffer);

		const uint32_t imageIndex = context.GetCurrentImageIndex();
		const auto& swapchainImages = context.GetSwapchainImages();

		KITA_CORE_ASSERT(imageIndex < swapchainImages.size(), "Swapchain image index out of range");

		TransitionSwapchainImageForPresent(commandBuffer, swapchainImages[imageIndex]);
		context.SetSwapchainImageLayout(imageIndex, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);
	}

	void VulkanRenderCommand::SetViewport(VkCommandBuffer commandBuffer, uint32_t width, uint32_t height)
	{
		KITA_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE, "VulkanRenderCommand command buffer is null");

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(width);
		viewport.height = static_cast<float>(height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
	}

	void VulkanRenderCommand::SetScissor(VkCommandBuffer commandBuffer, uint32_t width, uint32_t height)
	{
		KITA_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE, "VulkanRenderCommand command buffer is null");

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = { width, height };

		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
	}

	void VulkanRenderCommand::BindPipeline(VkCommandBuffer commandBuffer, const VulkanGraphicsPipeline& pipeline)
	{
		KITA_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE, "VulkanRenderCommand command buffer is null");
		KITA_CORE_ASSERT(pipeline.IsValid(), "VulkanGraphicsPipeline is invalid");

		pipeline.Bind(commandBuffer);
	}

	void VulkanRenderCommand::BindGeometry(VkCommandBuffer commandBuffer, const VulkanGeometry& geometry)
	{
		KITA_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE, "VulkanRenderCommand command buffer is null");

		VkBuffer vertexBuffers[] = { geometry.GetVertexBuffer() };
		VkDeviceSize offsets[] = { geometry.GetVertexBufferOffset() };

		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);

		if (geometry.HasIndices())
		{
			vkCmdBindIndexBuffer(
				commandBuffer,
				geometry.GetIndexBuffer(),
				geometry.GetIndexBufferOffset(),
				geometry.GetIndexType());
		}
	}

	void VulkanRenderCommand::DrawGeometry(VkCommandBuffer commandBuffer, const VulkanGeometry& geometry)
	{
		KITA_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE, "VulkanRenderCommand command buffer is null");

		if (geometry.HasIndices())
		{
			vkCmdDrawIndexed(
				commandBuffer,
				geometry.GetIndexCount(),
				1,
				0,
				0,
				0);
		}
		else
		{
			vkCmdDraw(
				commandBuffer,
				geometry.GetVertexCount(),
				1,
				0,
				0);
		}
	}

}
