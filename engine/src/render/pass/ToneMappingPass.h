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

		// 设置色调映射阶段采样的 HDR 源视图，统一 sampled input 的 view 级入口。
		void SetSourceInput(const VulkanRenderTargetView& sourceRenderTarget);
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
		VulkanRenderTargetView m_SourceRenderTarget;
		VulkanContext* m_Context = nullptr;
		std::vector<VulkanDescriptorSet> m_DescriptorSets;
	};

	RenderPassDesc MakeTonemappingPassDesc(const VulkanRenderTargetView& renderTarget);


}
