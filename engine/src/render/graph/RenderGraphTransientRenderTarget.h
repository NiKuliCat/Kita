#pragma once

#include "RenderGraph.h"
#include "RenderGraphResource.h"
#include "render/pass/RenderDataStruct.h"

#include <string>
#include <vector>


namespace Kita {


	struct RenderGraphColorAttachmentDesc
	{
		std::string Name;
		VkFormat Format = VK_FORMAT_UNDEFINED;
		VkImageUsageFlags ExtraUsage = 0;
		bool CreateSampler = true;
	};

	struct RenderGraphDepthAttachmentDesc
	{
		bool Enabled = true;
		std::string Name = "Depth";
		VkFormat Format = VK_FORMAT_D32_SFLOAT;
		VkImageUsageFlags ExtraUsage = 0;
		bool CreateSampler = true;
	};

	struct RenderGraphTransientRenderTargetDesc
	{
		std::string Name;
		uint32_t Width = 1;
		uint32_t Height = 1;
		VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;

		std::vector<RenderGraphColorAttachmentDesc> Colors;
		RenderGraphDepthAttachmentDesc Depth{};
	};



	struct RenderGraphTransientRenderTarget
	{
		std::vector<RenderGraphResourceID> ColorResourceIDs;
		std::vector<RenderGraphAttachmentRef> ColorAttachments;

		RenderGraphResourceID DepthResourceID = InvalidRenderGraphResourceID;
		RenderGraphAttachmentRef DepthAttachment{};

		bool HasDepth() const { return DepthAttachment.IsValid(); }
	};


	// 创建一组 transient image，并把它们组织成可传给 BuildRenderTargetView 的 attachment refs。
	// RenderGraph 不理解每张 color attachment 的内容，只负责资源声明和生命周期。
	inline RenderGraphTransientRenderTarget CreateTransientRenderTarget(
		RenderGraph& graph,
		const RenderGraphTransientRenderTargetDesc& desc)
	{
		RenderGraphTransientRenderTarget result{};
		result.ColorResourceIDs.reserve(desc.Colors.size());
		result.ColorAttachments.reserve(desc.Colors.size());

		for (uint32_t i = 0; i < static_cast<uint32_t>(desc.Colors.size()); ++i)
		{
			const RenderGraphColorAttachmentDesc& color = desc.Colors[i];

			RenderGraphTextureDesc textureDesc{};
			textureDesc.Name = desc.Name + "." + color.Name;
			textureDesc.Width = desc.Width;
			textureDesc.Height = desc.Height;
			textureDesc.Format = color.Format;
			textureDesc.Samples = desc.Samples;
			textureDesc.Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | color.ExtraUsage;

			if (color.CreateSampler)
				textureDesc.Usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

			textureDesc.CreateSampler = color.CreateSampler;

			RenderGraphResourceID id = graph.CreateTexture(textureDesc);
			result.ColorResourceIDs.push_back(id);
			result.ColorAttachments.push_back(RenderGraphAttachmentRef::MakeColor(id, 0));
		}

		if (desc.Depth.Enabled)
		{
			RenderGraphTextureDesc depthDesc{};
			depthDesc.Name = desc.Name + "." + desc.Depth.Name;
			depthDesc.Width = desc.Width;
			depthDesc.Height = desc.Height;
			depthDesc.Format = desc.Depth.Format;
			depthDesc.Samples = desc.Samples;
			depthDesc.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | desc.Depth.ExtraUsage;

			if (desc.Depth.CreateSampler)
				depthDesc.Usage |= VK_IMAGE_USAGE_SAMPLED_BIT;

			depthDesc.CreateSampler = desc.Depth.CreateSampler;

			result.DepthResourceID = graph.CreateTexture(depthDesc);
			result.DepthAttachment = RenderGraphAttachmentRef::MakeDepth(result.DepthResourceID);
		}

		return result;
	}

	inline RenderPassDesc MakeRenderPassDesc(
		const RenderGraphTransientRenderTargetDesc& targetDesc,
		const std::string& name,
		PassType type)
	{
		RenderPassDesc desc{};
		desc.Name = name;
		desc.Type = type;
		desc.Samples = targetDesc.Samples;
		desc.UseDepthAttachment = targetDesc.Depth.Enabled;

		desc.ColorFormats.reserve(targetDesc.Colors.size());
		for (const RenderGraphColorAttachmentDesc& color : targetDesc.Colors)
			desc.ColorFormats.push_back(color.Format);

		if (targetDesc.Depth.Enabled)
			desc.DepthFormat = targetDesc.Depth.Format;

		return desc;
	}
}
