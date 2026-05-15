#pragma once

#include <EngineCore.h>
#include <EngineRender.h>

namespace Kita {

	class EditorGridPass final : public FullscreenPassBase
	{
	public:
		struct alignas(16) PushConstants
		{
			glm::vec4 MinorColor = glm::vec4(0.42f, 0.44f, 0.46f, 0.32f);
			glm::vec4 MajorColor = glm::vec4(0.66f, 0.68f, 0.72f, 0.58f);
			glm::vec4 AxisXColor = glm::vec4(0.92f, 0.30f, 0.30f, 0.95f);
			glm::vec4 AxisZColor = glm::vec4(0.30f, 0.54f, 1.00f, 0.95f);
			glm::vec4 GridParams = glm::vec4(1.0f, 10.0f, 1.15f, 1.85f);
			glm::vec4 FadeParams = glm::vec4(1.0f, 120.0f, 0.02f, 0.005f);
		};

		static constexpr uint32_t PushConstantSize = sizeof(PushConstants);

	public:
		EditorGridPass(SceneBindings& sceneBindings, RenderPassDesc desc);

		void SetDepthDescriptorSet(VulkanDescriptorSet* descriptorSet) { m_DepthDescriptorSet = descriptorSet; }
		void SetPushConstants(const PushConstants& pushConstants) { m_PushConstants = pushConstants; }

	protected:
		void BindAdditionalResources(
			RenderPassContext& context,
			VkCommandBuffer commandBuffer,
			const VulkanGraphicsPipeline& pipeline,
			uint32_t frameIndex) override;

		void PushPassConstants(
			RenderPassContext& context,
			VkCommandBuffer commandBuffer,
			const VulkanGraphicsPipeline& pipeline) override;

	private:
		VulkanDescriptorSet* m_DepthDescriptorSet = nullptr;
		PushConstants m_PushConstants{};
	};

	RenderPassDesc MakeEditorGridPassDesc(const VulkanRenderTarget& renderTarget);

}
