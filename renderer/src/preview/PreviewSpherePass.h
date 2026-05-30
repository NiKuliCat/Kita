#pragma once

#include "render/pass/RenderPass.h"

namespace Kita {

	class VulkanGeometry;
	class VulkanGraphicsPipeline;
	class VulkanMaterial;

	struct PreviewSphereDrawItem
	{
		VulkanGraphicsPipeline* Pipeline = nullptr;
		VulkanGeometry* Geometry = nullptr;
		VulkanMaterial* Material = nullptr;
		ObjectData PerObject{};
	};

	class PreviewSpherePass : public SceneRenderPassBase
	{
	public:
		PreviewSpherePass(SceneBindings& sceneBindings, RenderPassDesc desc);

		void SetDrawItem(const PreviewSphereDrawItem& drawItem) { m_DrawItem = drawItem; }
		void Execute(RenderPassContext& context) override;

	private:
		PreviewSphereDrawItem m_DrawItem{};
	};

	RenderPassDesc MakePreviewSpherePassDesc(const VulkanRenderTargetView& renderTarget);

}
