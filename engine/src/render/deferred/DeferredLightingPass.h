#pragma once

#include "render/pass/FullscreenPassBase.h"

namespace Kita {

	class DeferredLightingPass final : public FullscreenPassBase
	{
	public:
		DeferredLightingPass(SceneBindings& sceneBindings, RenderPassDesc desc);
		~DeferredLightingPass() override;

		void Init(VulkanContext& context, uint32_t framesInFlight);
		void Destroy();

		void SetGBufferInput(const VulkanRenderTarget* gbufferRenderTarget);
		void UpdateFrameResources(uint32_t frameIndex);
		const VulkanDescriptorSet& GetDescriptorSet(uint32_t frameIndex) const { return m_DescriptorSets.at(frameIndex); }

	protected:
		void BindAdditionalResources(
			RenderPassContext& context,
			VkCommandBuffer commandBuffer,
			const VulkanGraphicsPipeline& pipeline,
			uint32_t frameIndex) override;

	private:
		void InitDescriptorSets(VulkanContext& context, uint32_t framesInFlight);
		void UpdateDescriptorSet(uint32_t frameIndex);

	private:
		const VulkanRenderTarget* m_GBufferRenderTarget = nullptr;
		VulkanContext* m_Context = nullptr;
		std::vector<VulkanDescriptorSet> m_DescriptorSets;
	};

	RenderPassDesc MakeDeferredLightingPassDesc(const VulkanRenderTarget& renderTarget);

}
