#include "kita_pch.h"
#include "FullscreenPassBase.h"
#include "SkyboxPass.h"
#include "render/VulkanContext.h"
#include "render/VulkanGraphicsPipeline.h"
#include "render/VulkanTexture.h"

namespace Kita {

	FullscreenPassBase::FullscreenPassBase(SceneBindings& sceneBindings, RenderPassDesc desc)
		: SceneRenderPassBase(sceneBindings, std::move(desc))
	{
	}

	void FullscreenPassBase::Execute(RenderPassContext& context)
	{
		if (!m_Pipeline || !m_Pipeline->IsValid())
			return;

		const ScenePassData& sceneData = GetSceneData();
		BeginPass(context, sceneData.BeginInfo);
		UpdateSceneBindings(context);

		const uint32_t frameIndex = context.GetFrameIndex();
		const VulkanDescriptorSet& sceneSet = GetSceneBindings().GetDescriptorSet(frameIndex);
		VkCommandBuffer commandBuffer = context.GetCommandBuffer();

		m_Pipeline->Bind(commandBuffer);
		sceneSet.Bind(commandBuffer, m_Pipeline->GetLayout(), 0);

		BindAdditionalResources(context, commandBuffer, *m_Pipeline, frameIndex);
		PushPassConstants(context, commandBuffer, *m_Pipeline);

		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		EndPass(context, sceneData.BeginInfo);
	}

	SkyboxPass::SkyboxPass(SceneBindings& sceneBindings, RenderPassDesc desc)
		: FullscreenPassBase(sceneBindings, std::move(desc))
	{
	}

	bool SkyboxPass::HasValidMaterial() const
	{
		if (!m_Material)
			return false;

		if (!m_Material->GetVertexShader() || !m_Material->GetFragmentShader())
			return false;

		const Ref<VulkanTexture>& texture = m_Material->GetAlbedoTexture();
		if (!texture || !texture->IsValid())
			return false;

		return texture->GetType() == TextureType::TextureCube;
	}

	void SkyboxPass::BindAdditionalResources(
		RenderPassContext& context,
		VkCommandBuffer commandBuffer,
		const VulkanGraphicsPipeline& pipeline,
		uint32_t frameIndex)
	{
		(void)context;

		if (!HasValidMaterial())
			return;

		if (!m_Material->HasDescriptorSets())
			return;

		m_Material->GetDescriptorSet(frameIndex).Bind(commandBuffer, pipeline.GetLayout(), 1);
	}

	RenderPassDesc MakeSkyboxPassDesc(const VulkanRenderTarget& renderTarget)
	{
		RenderPassDesc desc{};
		desc.Name = "SkyboxPass";
		desc.Type = PassType::PostProcess;
		desc.Samples = renderTarget.GetCreateInfo().Samples;
		desc.UseDepthAttachment = renderTarget.HasDepthAttachment();

		const uint32_t colorCount = renderTarget.GetColorAttachmentCount();
		desc.ColorFormats.reserve(colorCount);
		for (uint32_t i = 0; i < colorCount; ++i)
			desc.ColorFormats.push_back(renderTarget.GetColorFormat(i));

		if (renderTarget.HasDepthAttachment())
			desc.DepthFormat = renderTarget.GetDepthFormat();

		return desc;
	}

}
