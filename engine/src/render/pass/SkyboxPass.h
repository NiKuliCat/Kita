#pragma once
#include "FullscreenPassBase.h"
#include "render/VulkanMaterial.h"

namespace Kita {


	class SkyboxPass : public FullscreenPassBase
	{
	public:
		SkyboxPass(SceneBindings& sceneBindings, RenderPassDesc desc);

		void SetMaterial(const Ref<VulkanMaterial>& material) { m_Material = material; }
		const Ref<VulkanMaterial>& GetMaterial() const { return m_Material; }

		bool HasValidMaterial() const;

	protected:
		virtual void BindAdditionalResources(
			RenderPassContext& context,
			VkCommandBuffer commandBuffer,
			const VulkanGraphicsPipeline& pipeline,
			uint32_t frameIndex) override;

	private:
		Ref<VulkanMaterial> m_Material = nullptr;



	};

	RenderPassDesc MakeSkyboxPassDesc(const VulkanRenderTarget& renderTarget);
}