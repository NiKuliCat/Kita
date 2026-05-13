#pragma once
#include <EngineCore.h>
#include <EngineRender.h>
#include "ViewportCamera.h"
#include "ViewportPickingPass.h"

namespace Kita {

	class EditorViewportSurface;
	class EditorPickRegistry;

	class EditorRenderer
	{
	public:
		EditorRenderer(
			VulkanContext& context,
			VulkanRenderTarget& rt,
			VulkanRenderTarget& pickingRt,
			VulkanResourceFactory& vulkanResFactory,
			PipelineFactory& pipelineFactory,
			const Ref<Scene>& scene,
			ViewportCamera& camera,
			EditorPickRegistry& pickRegistry);

		void Init();
		void OnDestroy();
		void Render(EditorViewportSurface& surface);

	private:
		void InitRenderSceneData(ScenePassData& sceneData);
		VulkanGraphicsPipeline* GetPipeline(VulkanRenderTarget& rt, Ref<VulkanGeometry>& geometry, Ref<VulkanMaterial>& material);
		VulkanGraphicsPipeline* GetPickingPipeline(VulkanRenderTarget& rt, Ref<VulkanGeometry>& geometry);

	private:
		VulkanContext* m_Context = nullptr;
		SceneBindings m_SceneBindings;
		
		Unique<ForwardOpaquePass> m_ForwardOpaquePass;
		Unique<ViewportPickingPass> m_ViewportPickingPass;

		VulkanRenderTarget* m_RenderTarget = nullptr;
		Ref<Scene> m_SceneContext = nullptr;
		EditorPickRegistry* m_PickRegistry = nullptr;
		ViewportCamera* m_ViewportCamera = nullptr;
		VulkanResourceFactory* m_VulkanResFactory = nullptr;
		PipelineFactory*  m_PipelineFactory = nullptr;

	};

}
