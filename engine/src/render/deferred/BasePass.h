#pragma once
#include "render/pass/RenderPass.h"
#include "render/VulkanGeometry.h"
#include "render/VulkanGraphicsPipeline.h"
#include "render/VulkanMaterial.h"
namespace Kita {


	struct BasePassDrawItem
	{
		VulkanGraphicsPipeline* Pipeline = nullptr;
		VulkanGeometry* Geometry = nullptr;
		VulkanMaterial* Material = nullptr;
		ObjectData PerObject{};
	};


	class BasePass : public SceneRenderPassBase
	{
	public:
		BasePass(SceneBindings& bindings, RenderPassDesc desc);

		void ClearDrawItems();
		void AddDrawItem(const BasePassDrawItem& item);

		void Execute(RenderPassContext& context) override;

	private:
		std::vector<BasePassDrawItem> m_DrawItems{};

	};

	RenderPassDesc MakeBasePassDesc(const VulkanRenderTarget& renderTarget);

}