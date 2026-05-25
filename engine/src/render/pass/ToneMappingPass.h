#pragma once
#include "FullscreenPassBase.h"


namespace Kita {


	class ToneMappingPass final : public FullscreenPassBase
	{
	public:
		ToneMappingPass(SceneBindings& sceneBindings, RenderPassDesc desc);
		~ToneMappingPass() override;


		void Init(VulkanContext& context, uint32_t framesInFlight);
		void Destroy();

		void SetSourceInput(const VulkanRenderTarget* sourceRenderTarget);
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
		const VulkanRenderTarget* m_SourceRenderTarget = nullptr;
		VulkanContext* m_Context = nullptr;
		std::vector<VulkanDescriptorSet> m_DescriptorSets;
	};

	RenderPassDesc MakeTonemappingPassDesc(const VulkanRenderTarget& renderTarget);


}
