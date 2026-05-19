#include "renderer_pch.h"
#include "ViewportPickingPass.h"

#include "project/EditorProjectBootstrap.h"
#include "asset/AssetManager.h"
#include "asset/Asset.h"
#include "render/VulkanContext.h"
#include "render/VulkanRenderCommand.h"

namespace Kita {

	namespace
	{
		constexpr const char* kViewportPickingShaderName = "picking";

		void PushPickingData(
			VkCommandBuffer commandBuffer,
			VkPipelineLayout layout,
			const ViewportPickingPushConstants& pushConstants)
		{
			KITA_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE, "ViewportPickingPass command buffer is null");
			KITA_CORE_ASSERT(layout != VK_NULL_HANDLE, "ViewportPickingPass pipeline layout is null");

			vkCmdPushConstants(
				commandBuffer,
				layout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				ViewportPickingPushConstantSize,
				&pushConstants);
		}

		bool IsValidDrawItem(const ViewportPickingDrawItem& item)
		{
			return item.Pipeline != nullptr && item.Geometry != nullptr;
		}

	}

	ViewportPickingPass::ViewportPickingPass(SceneBindings& sceneBindings,RenderPassDesc desc)
		: SceneRenderPassBase(sceneBindings, std::move(desc))
	{

	}

	void ViewportPickingPass::ClearDrawItems()
	{
		m_DrawItems.clear();
	}

	void ViewportPickingPass::AddDrawItem(const ViewportPickingDrawItem& item)
	{
		m_DrawItems.push_back(item);
	}

	void ViewportPickingPass::Execute(RenderPassContext& context)
	{
		const ScenePassData& sceneData = GetSceneData();
		BeginPass(context, sceneData.BeginInfo);
		UpdateSceneBindings(context);

		const uint32_t frameIndex = context.GetFrameIndex();
		const VulkanDescriptorSet& sceneSet = GetSceneBindings().GetDescriptorSet(frameIndex);
		VkCommandBuffer commandBuffer = context.GetCommandBuffer();

		for (const ViewportPickingDrawItem& item : m_DrawItems)
		{
			if (!IsValidDrawItem(item))
				continue;

			if (!item.Pipeline->IsValid())
				continue;

			item.Pipeline->Bind(commandBuffer);
			sceneSet.Bind(commandBuffer, item.Pipeline->GetLayout(), 0);

			PushPickingData(commandBuffer, item.Pipeline->GetLayout(), item.PushConstants);
			VulkanRenderCommand::BindGeometry(commandBuffer, *item.Geometry);
			VulkanRenderCommand::DrawGeometry(commandBuffer, *item.Geometry);
		}

		EndPass(context, sceneData.BeginInfo);
	}


	RenderPassDesc MakeViewportPickingPassDesc(const VulkanRenderTarget& renderTarget)
	{
		RenderPassDesc desc{};
		desc.Name = "ViewportPickingPass";
		desc.Type = PassType::EditorPicking;
		desc.Samples = renderTarget.GetCreateInfo().Samples;

		const uint32_t colorCount = renderTarget.GetColorAttachmentCount();
		desc.ColorFormats.reserve(colorCount);

		for (uint32_t i = 0; i < colorCount; ++i)
			desc.ColorFormats.push_back(renderTarget.GetColorFormat(i));

		if (renderTarget.HasDepthAttachment())
			desc.DepthFormat = renderTarget.GetDepthFormat();

		return desc;
	}

}
