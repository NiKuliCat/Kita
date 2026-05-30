#pragma once
#include "RenderGraphResource.h"
#include "render/VulkanRenderTargetView.h"
#include <vulkan/vulkan.h>

namespace Kita {
	class RenderGraphAllocator;
	class VulkanContext;
	class VulkanImage;
	class VulkanRenderTarget;

	class RenderGraphContext
	{
	public:
		RenderGraphContext(
			VulkanContext& context,
			VkCommandBuffer commandBuffer,
			const std::vector<RenderGraphResource>& resources,
			RenderGraphAllocator& allocator);

		VulkanContext& GetVulkanContext() const { return *m_Context; }
		VkCommandBuffer GetCommandBuffer() const { return m_CommandBuffer; }

		const RenderGraphResource& GetResource(RenderGraphResourceID id) const;


		// 兼容旧路径：只支持 imported render target。
		VulkanRenderTarget& GetRenderTarget(RenderGraphResourceID id) const;
		VulkanRenderTargetView GetRenderTargetView(RenderGraphResourceID id) const;


		// 从 RenderGraph attachment refs 组装非拥有型 RT view。
		// 后续 transient Lighting / GBuffer 会优先走这个接口。
		VulkanRenderTargetView BuildRenderTargetView(
			const std::string& name,
			const std::vector<RenderGraphAttachmentRef>& colorAttachments,
			const RenderGraphAttachmentRef* depthAttachment = nullptr) const;


		// 获取用于 shader 读取/preview 的 attachment image。
		const VulkanImage& GetAttachmentImage(const RenderGraphAttachmentRef& attachment) const;

		VkDescriptorImageInfo GetAttachmentDescriptorInfo(
			const RenderGraphAttachmentRef& attachment,
			VkImageLayout samplerLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const;


		VulkanImage& GetTransientImage(RenderGraphResourceID id) const;
		const VulkanImage& GetTransientImageForPreview(RenderGraphResourceID id) const;

	private:
		// 获取用于 rendering attachment 的 image。Imported RT 的 color 会取真实 color attachment，
		// 而不是 sampled/resolve attachment。
		VulkanImage& GetAttachmentRenderImage(const RenderGraphAttachmentRef& attachment) const;

		// Imported MSAA RT 可能有 resolve image；transient texture 当前没有 resolve image。
		VulkanImage* GetAttachmentResolveImage(const RenderGraphAttachmentRef& attachment) const;

	private:
		VulkanContext* m_Context = nullptr;
		VkCommandBuffer m_CommandBuffer = VK_NULL_HANDLE;
		const std::vector<RenderGraphResource>* m_Resources = nullptr;
		RenderGraphAllocator* m_Allocator = nullptr;
	};

}
