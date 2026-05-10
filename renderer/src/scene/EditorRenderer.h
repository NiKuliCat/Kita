#pragma once
#include <EngineCore.h>
#include <EngineRender.h>
#include "ViewportCamera.h"
namespace Kita {

	class EditorRenderer
	{
	public:
		EditorRenderer(VulkanContext& context, VulkanRenderTarget& rt, const Ref<Scene>& scene, ViewportCamera& camera);

		void Init();
		void OnDestroy();
		void Render(VulkanRenderTarget& rt);

	private:
		void InitRenderSceneData(ScenePassData& sceneData);
		VulkanGraphicsPipeline* GetPipeline(VulkanRenderTarget& rt, Ref<VulkanGeometry>& geometry, Ref<VulkanMaterial>& material);

	private:
		VulkanContext* m_Context = nullptr;
		SceneBindings m_SceneBindings;
		VulkanResourceFactory m_VulkanResFactory;
		Unique<ForwardOpaquePass> m_ForwardOpaquePass;

		VulkanRenderTarget* m_RenderTarget = nullptr;
		Ref<Scene> m_SceneContext = nullptr;
		PipelineFactory  m_PipelineFactory;
		ViewportCamera* m_ViewportCamera = nullptr;
	};

}
