#pragma once
#include "FullscreenPassBase.h"
#include "render/VulkanMaterial.h"

namespace Kita {

	struct alignas(16) SkyboxPushConstants
	{
		float Intensity = 1.0f;
		float RotationY = 0.0f;
		float MipLevel = 0.0f;
		float Padding = 0.0f;
	};

	static constexpr uint32_t SkyboxPushConstantSize = sizeof(SkyboxPushConstants);

	class SkyboxPass : public FullscreenPassBase
	{
	public:
		SkyboxPass(SceneBindings& sceneBindings, RenderPassDesc desc);

		void SetMaterial(const Ref<VulkanMaterial>& material) { m_Material = material; }
		const Ref<VulkanMaterial>& GetMaterial() const { return m_Material; }
		void SetPushConstants(const SkyboxPushConstants& pushConstants) { m_PushConstants = pushConstants; }
		const SkyboxPushConstants& GetPushConstants() const { return m_PushConstants; }

		bool HasValidMaterial() const;

	protected:
		virtual void BindAdditionalResources(
			RenderPassContext& context,
			VkCommandBuffer commandBuffer,
			const VulkanGraphicsPipeline& pipeline,
			uint32_t frameIndex) override;

	private:
		Ref<VulkanMaterial> m_Material = nullptr;
		SkyboxPushConstants m_PushConstants{};

	};

	RenderPassDesc MakeSkyboxPassDesc(const VulkanRenderTarget& renderTarget);
}
