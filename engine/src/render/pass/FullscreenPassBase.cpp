#include "kita_pch.h"
#include "FullscreenPassBase.h"

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

}
