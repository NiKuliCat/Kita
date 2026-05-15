#pragma once

#include "RenderPass.h"
#include "render/VulkanGraphicsPipeline.h"

namespace Kita {

	class FullscreenPassBase : public SceneRenderPassBase
	{
	public:
		FullscreenPassBase(SceneBindings& sceneBindings, RenderPassDesc desc);
		virtual ~FullscreenPassBase() = default;

		void SetPipeline(VulkanGraphicsPipeline* pipeline) { m_Pipeline = pipeline; }

		void Execute(RenderPassContext& context) override;

	protected:
		virtual void BindAdditionalResources(
			RenderPassContext& context,
			VkCommandBuffer commandBuffer,
			const VulkanGraphicsPipeline& pipeline,
			uint32_t frameIndex) {}

		virtual void PushPassConstants(
			RenderPassContext& context,
			VkCommandBuffer commandBuffer,
			const VulkanGraphicsPipeline& pipeline) {}

	private:
		VulkanGraphicsPipeline* m_Pipeline = nullptr;
	};

}
