#pragma once

#include <EngineCore.h>
#include <EngineRender.h>

namespace Kita {

	struct alignas(16) ViewportPickingPushConstants
	{
		ObjectData PerObject{};
		uint32_t PickID = 0;
	};

	static constexpr uint32_t ViewportPickingPushConstantSize = sizeof(ViewportPickingPushConstants);

	struct ViewportPickingDrawItem
	{
		VulkanGraphicsPipeline* Pipeline = nullptr;
		VulkanGeometry* Geometry = nullptr;
		ViewportPickingPushConstants PushConstants{};
	};

	class ViewportPickingPass final : public SceneRenderPassBase
	{
	public:
		ViewportPickingPass(SceneBindings& sceneBindings,RenderPassDesc desc);

		void ClearDrawItems();
		void AddDrawItem(const ViewportPickingDrawItem& item);

		void Execute(RenderPassContext& context) override;

	private:
		std::vector<ViewportPickingDrawItem> m_DrawItems;
	};

	RenderPassDesc MakeViewportPickingPassDesc(const VulkanRenderTarget& renderTarget);

}
