#pragma once

#include "RenderGraphCompiled.h"
#include "RenderGraphResource.h"
#include "core/Core.h"
#include "render/VulkanImage.h"

#include <vector>

namespace Kita {

	class VulkanContext;

	// RenderGraph 临时资源分配器。
	// 当前第一版只为 TransientTexture 创建独立 VulkanImage，不做资源复用和 aliasing。
	class RenderGraphAllocator
	{
	public:
		void Reset();

		// 根据 graph 资源描述和编译结果创建本帧需要的 transient images。
		void Prepare(
			VulkanContext& context,
			const std::vector<RenderGraphResource>& resources,
			const RenderGraphCompileResult& compileResult);

		VulkanImage* GetTransientImage(RenderGraphResourceID id);
		const VulkanImage* GetTransientImage(RenderGraphResourceID id) const;
		void Dump(const std::vector<RenderGraphResource>& resources) const;

	private:
		struct TransientImageRecord
		{
			Unique<VulkanImage> Image = nullptr;
			RenderGraphTextureDesc Desc{};
			VkImageUsageFlags Usage = 0;
		};

		using TransientImageFrameRecords = std::vector<TransientImageRecord>;

	private:
		Unique<VulkanImage> CreateTransientImage(
			VulkanContext& context,
			const RenderGraphResource& resource,
			const RenderGraphResourceAccessSummary& accessSummary) const;

		bool NeedsRecreate(
			const TransientImageRecord& record,
			const RenderGraphTextureDesc& desc,
			VkImageUsageFlags usage) const;

		const TransientImageFrameRecords* GetCurrentFrameRecords() const;
		TransientImageFrameRecords* GetCurrentFrameRecords();

	private:
		std::vector<TransientImageFrameRecords> m_FrameTransientImages;
		uint32_t m_CurrentFrameIndex = 0;
	};

}
