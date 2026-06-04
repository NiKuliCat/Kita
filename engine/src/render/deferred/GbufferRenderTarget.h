#pragma once
#include "render/graph/RenderGraphTransientRenderTarget.h"

#include <algorithm>



namespace Kita {

	inline RenderGraphTransientRenderTargetDesc MakeDefaultGBufferRTargetDesc(
		uint32_t width,
		uint32_t height,
		VkSampleCountFlagBits samples,
		VkFormat depthFormat = VK_FORMAT_D32_SFLOAT)
	{
		RenderGraphTransientRenderTargetDesc desc{};
		desc.Name = "GBuffer";
		desc.Width = std::max(1u, width);
		desc.Height = std::max(1u, height);

		desc.Samples = samples;

		desc.Colors = {
			{"GBuffer0",VK_FORMAT_R8G8B8A8_UNORM},
			{"GBuffer1",VK_FORMAT_R8G8B8A8_UNORM},
			{"GBuffer2",VK_FORMAT_R16G16B16A16_SFLOAT },
			{"GBuffer3",VK_FORMAT_R16G16B16A16_SFLOAT },
			{"GBuffer4",VK_FORMAT_R16G16B16A16_SFLOAT }
		};
		desc.Depth.Enabled = true;
		desc.Depth.Name = "Depth";
		desc.Depth.Format = depthFormat;
		desc.Depth.CreateSampler = true;

		return desc;
	}


	// Lighting 默认是一张 HDR color + 一张 depth。
	// 这里只定义 RT 结构和格式，不负责实际 VulkanImage 创建。
	inline RenderGraphTransientRenderTargetDesc MakeDefaultLightingTargetDesc(
		uint32_t width,
		uint32_t height,
		VkSampleCountFlagBits samples,
		VkFormat depthFormat = VK_FORMAT_D32_SFLOAT)
	{
		RenderGraphTransientRenderTargetDesc desc{};
		desc.Name = "Lighting";
		desc.Width = std::max(1u, width);
		desc.Height = std::max(1u, height);
		desc.Samples = samples;

		desc.Colors = {
			{
				"Color0",
				VK_FORMAT_R16G16B16A16_SFLOAT,
				0,
				true
			}
		};

		desc.Depth.Enabled = true;
		desc.Depth.Name = "Depth";
		desc.Depth.Format = depthFormat;

		// Lighting depth 会被 CopyDepth 作为 transfer src 复制到最终视口 depth。
		desc.Depth.ExtraUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
		desc.Depth.CreateSampler = false;

		return desc;
	}
}
