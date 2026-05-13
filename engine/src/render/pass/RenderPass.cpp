#include "kita_pch.h"
#include "RenderPass.h"
#include "render/VulkanRenderCommand.h"
#include "core/Core.h"
#include "core/Log.h"
namespace Kita {


	RenderPassBase::RenderPassBase(RenderPassDesc desc)
		:m_Desc(std::move(desc))
	{

	}

	void RenderPassBase::ValidateRenderTarget(const VulkanRenderTarget& renderTarget) const
	{
		const uint32_t colorCount = renderTarget.GetColorAttachmentCount();
		KITA_CORE_ASSERT(colorCount == static_cast<uint32_t>(m_Desc.ColorFormats.size()),"RenderPass color attachment count does not match render target");

		for (uint32_t i = 0; i < colorCount; ++i)
		{
			KITA_CORE_ASSERT(renderTarget.GetColorFormat(i) == m_Desc.ColorFormats[i],"RenderPass color attachment format does not match render target");
		}
		if (m_Desc.DepthFormat != VK_FORMAT_UNDEFINED)
		{
			KITA_CORE_ASSERT(renderTarget.HasDepthAttachment(),"RenderPass requires depth attachment, but render target has none");

			KITA_CORE_ASSERT(renderTarget.GetDepthFormat() == m_Desc.DepthFormat,"RenderPass depth format does not match render target");
		}
	}



	void RenderPassBase::BeginPass(RenderPassContext& context, const RenderPassBeginInfo& beginInfo) const
	{
		VulkanRenderTarget& rt = context.GetRenderTarget();
		ValidateRenderTarget(rt);

		std::vector<VkClearValue> colorClearValues;
		colorClearValues.reserve(rt.GetColorAttachmentCount());

		for (uint32_t i = 0; i < rt.GetColorAttachmentCount(); ++i)
		{
			colorClearValues.push_back(
				VulkanRenderTarget::MakeColorClearValue(
					beginInfo.ClearColor.r,
					beginInfo.ClearColor.g,
					beginInfo.ClearColor.b,
					beginInfo.ClearColor.a));
		}

		VkClearValue depthClearValue =
			VulkanRenderTarget::MakeDepthClearValue(
				beginInfo.ClearDepth,
				beginInfo.ClearStencil);

		const VkClearValue* depthClearPtr = nullptr;
		if (rt.HasDepthAttachment())
			depthClearPtr = &depthClearValue;

		rt.BeginRendering(
			context.GetCommandBuffer(),
			colorClearValues,
			depthClearPtr);

		VulkanRenderCommand::SetViewport(
			context.GetCommandBuffer(),
			rt.GetWidth(),
			rt.GetHeight());

		VulkanRenderCommand::SetScissor(
			context.GetCommandBuffer(),
			rt.GetWidth(),
			rt.GetHeight());
	}

	void RenderPassBase::EndPass(RenderPassContext& context, const RenderPassBeginInfo& beginInfo) const
	{
		context.GetRenderTarget().EndRendering(
			context.GetCommandBuffer(),
			beginInfo.TransitionSampledColors,
			beginInfo.TransitionSampledDepth);
	}

	SceneRenderPassBase::SceneRenderPassBase(SceneBindings& sceneBindings, RenderPassDesc desc)
		:RenderPassBase(std::move(desc)), m_SceneBindings(&sceneBindings)
	{
		KITA_CORE_ASSERT(m_SceneBindings, "SceneRenderPassBase scene bindings is null");
	}

	void SceneRenderPassBase::UpdateSceneBindings(RenderPassContext& context)
	{
		KITA_CORE_ASSERT(m_SceneBindings, "SceneRenderPassBase scene bindings is null");

		m_SceneBindings->Update(
			context.GetFrameIndex(),
			m_SceneData.Camera,
			m_SceneData.MainLight);
	}

}