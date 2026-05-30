#include "kita_pch.h"
#include "RenderGraphAllocator.h"

#include "core/Core.h"
#include "core/Log.h"
#include "render/VulkanContext.h"

namespace Kita {

	void RenderGraphAllocator::Reset()
	{
		m_FrameTransientImages.clear();
		m_CurrentFrameIndex = 0;
	}

	void RenderGraphAllocator::Prepare(
		VulkanContext& context,
		const std::vector<RenderGraphResource>& resources,
		const RenderGraphCompileResult& compileResult)
	{
		KITA_CORE_ASSERT(
			compileResult.Valid,
			"RenderGraphAllocator requires a valid compile result");

		KITA_CORE_ASSERT(
			compileResult.ResourceAccessSummaries.size() == resources.size(),
			"RenderGraphAllocator resource summary count mismatch");

		const uint32_t framesInFlight = std::max(1u, context.GetFramesInFlight());
		const uint32_t currentFrameIndex = context.GetCurrentFrameIndex() % framesInFlight;

		if (m_FrameTransientImages.size() != framesInFlight)
			m_FrameTransientImages.resize(framesInFlight);

		m_CurrentFrameIndex = currentFrameIndex;
		TransientImageFrameRecords& transientImages = m_FrameTransientImages[m_CurrentFrameIndex];
		transientImages.resize(resources.size());

		for (RenderGraphResourceID id = 0; id < static_cast<RenderGraphResourceID>(resources.size()); ++id)
		{
			const RenderGraphResource& resource = resources[id];

			if (resource.Type != RenderGraphResourceType::TransientTexture)
			{
				transientImages[id] = {};
				continue;
			}

			const RenderGraphResourceAccessSummary& accessSummary =
				compileResult.ResourceAccessSummaries[id];
			const RenderGraphTextureDesc& desc = resource.TextureDesc;
			const VkImageUsageFlags usage = accessSummary.Usage | desc.Usage;

			KITA_CORE_ASSERT(
				usage != 0,
				"RenderGraph transient texture has empty image usage");

			TransientImageRecord& record = transientImages[id];
			if (NeedsRecreate(record, desc, usage))
			{
				record.Image = CreateTransientImage(context, resource, accessSummary);
				record.Desc = desc;
				record.Usage = usage;
			}
		}
	}

	VulkanImage* RenderGraphAllocator::GetTransientImage(RenderGraphResourceID id)
	{
		TransientImageFrameRecords* transientImages = GetCurrentFrameRecords();
		if (!transientImages || id == InvalidRenderGraphResourceID || id >= transientImages->size())
			return nullptr;

		return (*transientImages)[id].Image.get();
	}

	const VulkanImage* RenderGraphAllocator::GetTransientImage(RenderGraphResourceID id) const
	{
		const TransientImageFrameRecords* transientImages = GetCurrentFrameRecords();
		if (!transientImages || id == InvalidRenderGraphResourceID || id >= transientImages->size())
			return nullptr;

		return (*transientImages)[id].Image.get();
	}

	void RenderGraphAllocator::Dump(const std::vector<RenderGraphResource>& resources) const
	{
		KITA_CORE_TRACE(
			"  Transient images: frameSlot={} frameSlots={}",
			m_CurrentFrameIndex,
			m_FrameTransientImages.size());

		const TransientImageFrameRecords* transientImages = GetCurrentFrameRecords();
		if (!transientImages)
			return;

		for (RenderGraphResourceID id = 0; id < static_cast<RenderGraphResourceID>(transientImages->size()); ++id)
		{
			if (id >= resources.size() || resources[id].Type != RenderGraphResourceType::TransientTexture)
				continue;

			const TransientImageRecord& record = (*transientImages)[id];
			const VulkanImage* image = record.Image.get();

			if (!image)
			{
				KITA_CORE_TRACE("    [{}] {} image=null", id, resources[id].Name);
				continue;
			}

			const VkExtent3D extent = image->GetExtent();
			KITA_CORE_TRACE(
				"    [{}] {} valid={} extent={}x{}x{} format={} usage={} sampler={} handle={}",
				id,
				resources[id].Name,
				image->IsValid(),
				extent.width,
				extent.height,
				extent.depth,
				static_cast<uint32_t>(image->GetFormat()),
				static_cast<uint32_t>(image->GetUsage()),
				image->HasSampler(),
				reinterpret_cast<uint64_t>(image->GetHandle()));
		}
	}

	Unique<VulkanImage> RenderGraphAllocator::CreateTransientImage(
		VulkanContext& context,
		const RenderGraphResource& resource,
		const RenderGraphResourceAccessSummary& accessSummary) const
	{
		KITA_CORE_ASSERT(
			resource.Type == RenderGraphResourceType::TransientTexture,
			"RenderGraphAllocator can only create transient texture images");

		const RenderGraphTextureDesc& desc = resource.TextureDesc;

		VulkanImage::CreateInfo imageInfo{};
		imageInfo.Name = desc.Name;
		imageInfo.Type = VK_IMAGE_TYPE_2D;
		imageInfo.Format = desc.Format;
		imageInfo.Extent = { desc.Width, desc.Height, 1 };
		imageInfo.MipLevels = 1;
		imageInfo.ArrayLayers = 1;
		imageInfo.Samples = desc.Samples;
		imageInfo.Usage = accessSummary.Usage | desc.Usage;
		imageInfo.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		imageInfo.CreateSampler = desc.CreateSampler;

		if ((imageInfo.Usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0)
			imageInfo.AspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT;

		return CreateUnique<VulkanImage>(context, imageInfo);
	}

	bool RenderGraphAllocator::NeedsRecreate(
		const TransientImageRecord& record,
		const RenderGraphTextureDesc& desc,
		VkImageUsageFlags usage) const
	{
		if (!record.Image || !record.Image->IsValid())
			return true;

		return record.Desc.Width != desc.Width ||
			record.Desc.Height != desc.Height ||
			record.Desc.Format != desc.Format ||
			record.Desc.Samples != desc.Samples ||
			record.Desc.CreateSampler != desc.CreateSampler ||
			record.Usage != usage;
	}

	const RenderGraphAllocator::TransientImageFrameRecords* RenderGraphAllocator::GetCurrentFrameRecords() const
	{
		if (m_CurrentFrameIndex >= m_FrameTransientImages.size())
			return nullptr;

		return &m_FrameTransientImages[m_CurrentFrameIndex];
	}

	RenderGraphAllocator::TransientImageFrameRecords* RenderGraphAllocator::GetCurrentFrameRecords()
	{
		if (m_CurrentFrameIndex >= m_FrameTransientImages.size())
			return nullptr;

		return &m_FrameTransientImages[m_CurrentFrameIndex];
	}

}
