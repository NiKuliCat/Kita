#pragma once
#include "RenderPass.h"
#include "render/VulkanGeometry.h"
#include "render/VulkanGraphicsPipeline.h"
#include "render/VulkanMaterial.h"
namespace Kita {

    struct ForwardOpaqueDrawItem
    {
        VulkanGraphicsPipeline* Pipeline = nullptr;
        VulkanGeometry* Geometry = nullptr;
        VulkanMaterial* Material = nullptr;
        ObjectData PerObject{};
    };

	class ForwardOpaquePass final : public SceneRenderPassBase
	{
    public:
        ForwardOpaquePass(SceneBindings& sceneBindings, RenderPassDesc desc);

        void ClearDrawItems();
        void AddDrawItem(const ForwardOpaqueDrawItem& item);

        void Execute(RenderPassContext& context) override;

    private:
        std::vector<ForwardOpaqueDrawItem> m_DrawItems;
	};

    RenderPassDesc MakeForwardOpaquePassDesc(const VulkanRenderTarget& renderTarget);
}