#pragma once
#include <EngineCore.h>
#include <EngineRender.h>
#include "ViewportCamera.h"
#include "EditorGridPass.h"
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
		void SetGridEnabled(bool enabled) { m_IsGridEnabled = enabled; }
		bool IsGridEnabled() const { return m_IsGridEnabled; }
		EditorGridPass::PushConstants& GetGridPushConstants() { return m_GridPushConstants; }
		const EditorGridPass::PushConstants& GetGridPushConstants() const { return m_GridPushConstants; }
		void SetGridPushConstants(const EditorGridPass::PushConstants& pushConstants) { m_GridPushConstants = pushConstants; }

	private:
		void InitRenderSceneData(ScenePassData& sceneData);
		void InitGridResources();
		void UpdateGridDepthBinding(VulkanRenderTarget& rt);
		VulkanGraphicsPipeline* GetPipeline(VulkanRenderTarget& rt, Ref<VulkanGeometry>& geometry, Ref<VulkanMaterial>& material);
		VulkanGraphicsPipeline* GetGridPipeline(VulkanRenderTarget& rt);
		VulkanGraphicsPipeline* GetPickingPipeline(VulkanRenderTarget& rt, Ref<VulkanGeometry>& geometry);

	private:
		VulkanContext* m_Context = nullptr;
		SceneBindings m_SceneBindings;
		
		Unique<ForwardOpaquePass> m_ForwardOpaquePass;
		Unique<EditorGridPass> m_EditorGridPass;
		Unique<ViewportPickingPass> m_ViewportPickingPass;
		Unique<VulkanDescriptorSet> m_GridDepthDescriptorSet;
		Ref<VulkanShader> m_GridVertexShader = nullptr;
		Ref<VulkanShader> m_GridFragmentShader = nullptr;
		EditorGridPass::PushConstants m_GridPushConstants{};
		bool m_IsGridEnabled = true;

		VulkanRenderTarget* m_RenderTarget = nullptr;
		Ref<Scene> m_SceneContext = nullptr;
		EditorPickRegistry* m_PickRegistry = nullptr;
		ViewportCamera* m_ViewportCamera = nullptr;
		VulkanResourceFactory* m_VulkanResFactory = nullptr;
		PipelineFactory*  m_PipelineFactory = nullptr;

	};

}
