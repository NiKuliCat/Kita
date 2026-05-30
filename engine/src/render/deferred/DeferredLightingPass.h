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

		// 设置延迟光照阶段采样的 GBuffer 视图，避免 pass 直接依赖完整 RenderTarget 实例。
		void SetGBufferInput(const VulkanRenderTargetView& gbufferRenderTarget);
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
		VulkanRenderTargetView m_GBufferRenderTarget;
		Ref<ImageBasedLighting> m_IBL = nullptr;
		// 始终准备一套默认 IBL 纹理，避免未 Bake 时 descriptor 未写入。
		Ref<VulkanTexture> m_FallbackIrradianceCube = nullptr;
		Ref<VulkanTexture> m_FallbackPrefilterCube = nullptr;
		Ref<VulkanTexture> m_FallbackBrdfLut = nullptr;
		Ref<VulkanTexture> m_FallbackEnvironmentCube = nullptr;
		VulkanContext* m_Context = nullptr;
		std::vector<VulkanDescriptorSet> m_DescriptorSets;
	};

	RenderPassDesc MakeDeferredLightingPassDesc(const VulkanRenderTargetView& renderTarget);

}
