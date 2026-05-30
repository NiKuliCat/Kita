#include "renderer_pch.h"
#include "PreviewSpherePass.h"

#include "render/VulkanGeometry.h"
#include "render/VulkanGraphicsPipeline.h"
#include "render/VulkanMaterial.h"
#include "render/VulkanRenderCommand.h"

namespace Kita {

	namespace
	{
		bool IsValidDrawItem(const PreviewSphereDrawItem& item)
		{
			return item.Pipeline &&
				item.Pipeline->IsValid() &&
				item.Geometry &&
				item.Material &&
				item.Material->HasDescriptorSets();
		}

		void PushObjectData(VkCommandBuffer commandBuffer, VkPipelineLayout layout, const ObjectData& objectData)
		{
			KITA_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE, "PreviewSpherePass command buffer is null");
			KITA_CORE_ASSERT(layout != VK_NULL_HANDLE, "PreviewSpherePass pipeline layout is null");

			vkCmdPushConstants(
				commandBuffer,
				layout,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
				0,
				ObjectDataSize,
				&objectData);
		}
	}

	PreviewSpherePass::PreviewSpherePass(SceneBindings& sceneBindings, RenderPassDesc desc)
		: SceneRenderPassBase(sceneBindings, std::move(desc))
	{
	}

	void PreviewSpherePass::Execute(RenderPassContext& context)
	{
		if (!IsValidDrawItem(m_DrawItem))
		{
			return;
		}

		const ScenePassData& sceneData = GetSceneData();
		BeginPass(context, sceneData.BeginInfo);
		UpdateSceneBindings(context);

		const uint32_t frameIndex = context.GetFrameIndex();
		const VulkanDescriptorSet& sceneSet = GetSceneBindings().GetDescriptorSet(frameIndex);
		VkCommandBuffer commandBuffer = context.GetCommandBuffer();

		m_DrawItem.Pipeline->Bind(commandBuffer);
		sceneSet.Bind(commandBuffer, m_DrawItem.Pipeline->GetLayout(), 0);
		m_DrawItem.Material->GetDescriptorSet(frameIndex).Bind(commandBuffer, m_DrawItem.Pipeline->GetLayout(), 1);
		PushObjectData(commandBuffer, m_DrawItem.Pipeline->GetLayout(), m_DrawItem.PerObject);
		VulkanRenderCommand::BindGeometry(commandBuffer, *m_DrawItem.Geometry);
		VulkanRenderCommand::DrawGeometry(commandBuffer, *m_DrawItem.Geometry);

		EndPass(context, sceneData.BeginInfo);
	}

	RenderPassDesc MakePreviewSpherePassDesc(const VulkanRenderTargetView& renderTarget)
	{
		RenderPassDesc desc{};
		desc.Name = "PreviewSpherePass";
		desc.Type = PassType::ForwardOpaque;
		desc.Samples = renderTarget.GetSamples();
		desc.UseDepthAttachment = renderTarget.HasDepthAttachment();

		const uint32_t colorCount = renderTarget.GetColorAttachmentCount();
		desc.ColorFormats.reserve(colorCount);
		for (uint32_t i = 0; i < colorCount; ++i)
		{
			desc.ColorFormats.push_back(renderTarget.GetColorFormat(i));
		}

		if (renderTarget.HasDepthAttachment())
		{
			desc.DepthFormat = renderTarget.GetDepthFormat();
		}

		return desc;
	}

}
