#include "kita_pch.h"
#include "RenderGraphContext.h"
#include "RenderGraphAllocator.h"
#include "render/VulkanImage.h"
#include "render/VulkanRenderTarget.h"
#include "core/Log.h"
namespace Kita {



	RenderGraphContext::RenderGraphContext(
		VulkanContext& context,
		VkCommandBuffer commandBuffer,
		const std::vector<RenderGraphResource>& resources,
		RenderGraphAllocator& allocator)
		:m_Context(&context),m_CommandBuffer(commandBuffer),m_Resources(&resources),m_Allocator(&allocator)
	{
	}

	const RenderGraphResource& RenderGraphContext::GetResource(RenderGraphResourceID id) const
	{
		KITA_CORE_ASSERT(m_Resources && id < m_Resources->size(), "RenderGraph resource id is invalid");
		return m_Resources->at(id);
	}

	VulkanRenderTarget& RenderGraphContext::GetRenderTarget(RenderGraphResourceID id) const
	{
		const RenderGraphResource& resource = GetResource(id);
		KITA_CORE_ASSERT(
			resource.Type == RenderGraphResourceType::ImportedRenderTarget,
			"RenderGraph resource is not an imported render target");

		KITA_CORE_ASSERT(resource.RT, "RenderGraph imported render target is null");

		return *resource.RT;
	}

	VulkanRenderTargetView RenderGraphContext::GetRenderTargetView(RenderGraphResourceID id) const
	{
		const RenderGraphResource& resource = GetResource(id);
		KITA_CORE_ASSERT(
			resource.Type == RenderGraphResourceType::ImportedRenderTarget,
			"RenderGraph resource is not an imported render target");

		KITA_CORE_ASSERT(resource.RT, "RenderGraph imported render target is null");
		return resource.RT->CreateView();
	}

	VulkanRenderTargetView RenderGraphContext::BuildRenderTargetView(const std::string& name, const std::vector<RenderGraphAttachmentRef>& colorAttachments, const RenderGraphAttachmentRef* depthAttachment) const
	{
		KITA_CORE_ASSERT(m_Context, "RenderGraphContext VulkanContext is null");
		KITA_CORE_ASSERT(!colorAttachments.empty(), "RenderGraph render target view requires at least one color attachment");


		VulkanRenderTargetView::CreateInfo createInfo{};
		createInfo.Name = name;
		createInfo.Context = m_Context;

		const VulkanImage& firstColor = GetAttachmentRenderImage(colorAttachments[0]);
		const VkExtent3D extent = firstColor.GetExtent();

		createInfo.Width = extent.width;
		createInfo.Height = extent.height;
		createInfo.Samples = firstColor.GetSamples();

		createInfo.ColorAttachments.reserve(colorAttachments.size());

		for (const RenderGraphAttachmentRef& attachment : colorAttachments)
		{
			KITA_CORE_ASSERT(
				attachment.Type == RenderGraphAttachmentType::Color,
				"RenderGraph color render target view requires color attachment refs");

			VulkanImage& colorImage = GetAttachmentRenderImage(attachment);
			const VkExtent3D colorExtent = colorImage.GetExtent();

			KITA_CORE_ASSERT(colorExtent.width == createInfo.Width, "RenderGraph RT view color attachment width mismatch");
			KITA_CORE_ASSERT(colorExtent.height == createInfo.Height, "RenderGraph RT view color attachment height mismatch");
			KITA_CORE_ASSERT(colorImage.GetSamples() == createInfo.Samples, "RenderGraph RT view color attachment sample mismatch");

			VulkanRenderTargetView::ColorAttachmentView colorView{};
			colorView.Image = &colorImage;
			colorView.ResolveImage = GetAttachmentResolveImage(attachment);
			colorView.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorView.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;

			createInfo.ColorAttachments.push_back(colorView);
		}


		if (depthAttachment && depthAttachment->IsValid())
		{
			KITA_CORE_ASSERT(
				depthAttachment->Type == RenderGraphAttachmentType::Depth,
				"RenderGraph depth render target view requires a depth attachment ref");

			VulkanImage& depthImage = GetAttachmentRenderImage(*depthAttachment);
			const VkExtent3D depthExtent = depthImage.GetExtent();

			KITA_CORE_ASSERT(depthExtent.width == createInfo.Width, "RenderGraph RT view depth attachment width mismatch");
			KITA_CORE_ASSERT(depthExtent.height == createInfo.Height, "RenderGraph RT view depth attachment height mismatch");
			KITA_CORE_ASSERT(depthImage.GetSamples() == createInfo.Samples, "RenderGraph RT view depth attachment sample mismatch");

			createInfo.DepthAttachment.Image = &depthImage;
			createInfo.DepthAttachment.LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			createInfo.DepthAttachment.StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		}

		return VulkanRenderTargetView(createInfo);
	}

	const VulkanImage& RenderGraphContext::GetAttachmentImage(const RenderGraphAttachmentRef& attachment) const
	{
		KITA_CORE_ASSERT(attachment.IsValid(), "RenderGraph attachment ref is invalid");

		const RenderGraphResource& resource = GetResource(attachment.Resource);
		if (resource.Type == RenderGraphResourceType::ImportedRenderTarget)
		{
			KITA_CORE_ASSERT(resource.RT, "RenderGraph imported render target is null");

			if (attachment.Type == RenderGraphAttachmentType::Depth)
			{
				const VulkanImage* depthAttachment = resource.RT->GetDepthAttachment();
				KITA_CORE_ASSERT(depthAttachment, "RenderGraph imported render target has no depth attachment");
				return *depthAttachment;
			}

			return resource.RT->GetSampledColorAttachment(attachment.Index);
		}

		KITA_CORE_ASSERT(
			resource.Type == RenderGraphResourceType::TransientTexture,
			"RenderGraph attachment ref targets unsupported resource type");
		KITA_CORE_ASSERT(
			attachment.Type == RenderGraphAttachmentType::Color,
			"RenderGraph transient texture does not support depth attachment view");

		return GetTransientImage(attachment.Resource);
	}

	VkDescriptorImageInfo RenderGraphContext::GetAttachmentDescriptorInfo(
		const RenderGraphAttachmentRef& attachment,
		VkImageLayout samplerLayout) const
	{
		return GetAttachmentImage(attachment).GetDescriptorInfo(samplerLayout);
	}

	VulkanImage& RenderGraphContext::GetTransientImage(RenderGraphResourceID id) const
	{
		const RenderGraphResource& resource = GetResource(id);
		KITA_CORE_ASSERT(
			resource.Type == RenderGraphResourceType::TransientTexture,
			"RenderGraph resource is not a transient texture");

		KITA_CORE_ASSERT(m_Allocator, "RenderGraph allocator is null");

		VulkanImage* image = m_Allocator->GetTransientImage(id);
		KITA_CORE_ASSERT(image, "RenderGraph transient image is null");

		return *image;
	}

	const VulkanImage& RenderGraphContext::GetTransientImageForPreview(RenderGraphResourceID id) const
	{
		return GetTransientImage(id);
	}

	VulkanImage& RenderGraphContext::GetAttachmentRenderImage(const RenderGraphAttachmentRef& attachment) const
	{
		KITA_CORE_ASSERT(attachment.IsValid(), "RenderGraph attachment ref is invalid");

		const RenderGraphResource& resource = GetResource(attachment.Resource);
		if (resource.Type == RenderGraphResourceType::ImportedRenderTarget)
		{
			KITA_CORE_ASSERT(resource.RT, "RenderGraph imported render target is null");

			if (attachment.Type == RenderGraphAttachmentType::Depth)
			{
				const VulkanImage* depthAttachment = resource.RT->GetDepthAttachment();
				KITA_CORE_ASSERT(depthAttachment, "RenderGraph imported render target has no depth attachment");
				return const_cast<VulkanImage&>(*depthAttachment);
			}

			return const_cast<VulkanImage&>(resource.RT->GetColorAttachment(attachment.Index));
		}

		KITA_CORE_ASSERT(
			resource.Type == RenderGraphResourceType::TransientTexture,
			"RenderGraph attachment ref targets unsupported resource type");

		return GetTransientImage(attachment.Resource);
	}

	VulkanImage* RenderGraphContext::GetAttachmentResolveImage(const RenderGraphAttachmentRef& attachment) const
	{
		if (attachment.Type != RenderGraphAttachmentType::Color)
			return nullptr;

		const RenderGraphResource& resource = GetResource(attachment.Resource);
		if (resource.Type != RenderGraphResourceType::ImportedRenderTarget)
			return nullptr;

		KITA_CORE_ASSERT(resource.RT, "RenderGraph imported render target is null");

		const VulkanImage* resolveImage = resource.RT->GetResolveAttachment(attachment.Index);
		return const_cast<VulkanImage*>(resolveImage);
	}

}
