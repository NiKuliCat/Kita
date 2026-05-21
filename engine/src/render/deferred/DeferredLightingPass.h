#pragma once

#include "render/pass/FullscreenPassBase.h"
#include "render/ibl/IBLGenerator.h"
namespace Kita {

	class DeferredLightingPass final : public FullscreenPassBase
	{
	public:
		DeferredLightingPass(SceneBindings& sceneBindings, RenderPassDesc desc);
		~DeferredLightingPass() override;

		void Init(VulkanContext& context, uint32_t framesInFlight);
		void Destroy();

		void SetGBufferInput(const VulkanRenderTarget* gbufferRenderTarget);
		void SetIBLInput(const Ref<ImageBasedLighting>& ibl) { m_IBL = ibl; }
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
		Ref<ImageBasedLighting> m_IBL = nullptr;
		// 始终准备一套默认 IBL 纹理，避免未 Bake 时 descriptor 未写入。
		Ref<VulkanTexture> m_FallbackIrradianceCube = nullptr;
		Ref<VulkanTexture> m_FallbackPrefilterCube = nullptr;
		Ref<VulkanTexture> m_FallbackBrdfLut = nullptr;
		Ref<VulkanTexture> m_FallbackEnvironmentCube = nullptr;
		VulkanContext* m_Context = nullptr;
		std::vector<VulkanDescriptorSet> m_DescriptorSets;
	};

	RenderPassDesc MakeDeferredLightingPassDesc(const VulkanRenderTarget& renderTarget);

}
