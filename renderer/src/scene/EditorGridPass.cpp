#include "renderer_pch.h"
#include "EditorGridPass.h"

namespace Kita {

	EditorGridPass::EditorGridPass(SceneBindings& sceneBindings, RenderPassDesc desc)
		: FullscreenPassBase(sceneBindings, std::move(desc))
	{
	}

	void EditorGridPass::PushPassConstants(
		RenderPassContext& context,
		VkCommandBuffer commandBuffer,
		const VulkanGraphicsPipeline& pipeline)
	{
		(void)context;

		vkCmdPushConstants(
			commandBuffer,
			pipeline.GetLayout(),
			VK_SHADER_STAGE_FRAGMENT_BIT,
			0,
			PushConstantSize,
			&m_PushConstants);
	}

	RenderPassDesc MakeEditorGridPassDesc(const VulkanRenderTarget& renderTarget)
	{
		RenderPassDesc desc{};
		desc.Name = "EditorGridPass";
		desc.Type = PassType::PostProcess;
		desc.Samples = VK_SAMPLE_COUNT_1_BIT;
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
