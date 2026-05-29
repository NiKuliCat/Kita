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
			VulkanRenderTarget& gbufferRt,
			VulkanRenderTarget& lightingRt,
			VulkanRenderTarget& finalRt,
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

		void SetIBLSource(const Ref<ImageBasedLighting>& ibl) { m_IBL = ibl; }
	private:
		void InitRenderSceneData(ScenePassData& sceneData);
		void InitGridResources();
		void InitDeferredLightingResources();
		void InitTonemapResources();
		void SyncSkyboxMaterialFromSettings();

		void BuildRenderGraph(EditorViewportSurface& surface,
			VulkanRenderTarget& gbufferRt,
			VulkanRenderTarget& lightingRt,
			VulkanRenderTarget& finalRt,
			VulkanRenderTarget& pickingRt);


		VulkanGraphicsPipeline* GetPipeline(VulkanRenderTarget& rt, Ref<VulkanGeometry>& geometry, Ref<VulkanMaterial>& material);
		VulkanGraphicsPipeline* GetDeferredLightingPipeline(VulkanRenderTarget& rt);
		VulkanGraphicsPipeline* GetTonemapPipeline(VulkanRenderTarget& rt);
		VulkanGraphicsPipeline* GetGridPipeline(VulkanRenderTarget& rt);
		VulkanGraphicsPipeline* GetPickingPipeline(VulkanRenderTarget& rt, Ref<VulkanGeometry>& geometry);
		VulkanGraphicsPipeline* GetSkyboxPipeline(VulkanRenderTarget& rt);
		void CopyDepthAttachment(
			const VulkanRenderTarget& sourceRt,
			VulkanRenderTarget& targetRt,
			VkCommandBuffer commandBuffer) const;
	private:
		VulkanContext* m_Context = nullptr;
		SceneBindings m_SceneBindings;
		
		Unique<BasePass> m_BasePass;
		Unique<DeferredLightingPass> m_DeferredLightingPass;
		Unique<ForwardOpaquePass> m_ForwardOpaquePass;
		Unique<ToneMappingPass> m_TonemapPass;
		Unique<EditorGridPass> m_EditorGridPass;
		Unique<ViewportPickingPass> m_ViewportPickingPass;
		Unique<SkyboxPass> m_SkyboxPass;

		Ref<VulkanMaterial> m_SkyboxMaterial = nullptr;
		Ref<VulkanTexture> m_DefaultSkyboxTexture = nullptr;
		Ref<VulkanShader> m_GridVertexShader = nullptr;
		Ref<VulkanShader> m_GridFragmentShader = nullptr;
		Ref<VulkanShader> m_DeferredLightingVertexShader = nullptr;
		Ref<VulkanShader> m_DeferredLightingFragmentShader = nullptr;
		Ref<VulkanShader> m_TonemapVertexShader = nullptr;
		Ref<VulkanShader> m_TonemapFragmentShader = nullptr;
		EditorGridPass::PushConstants m_GridPushConstants{};
		bool m_IsGridEnabled = true;

		VulkanRenderTarget* m_GBufferRenderTarget = nullptr;
		VulkanRenderTarget* m_LightingRenderTarget = nullptr;
		VulkanRenderTarget* m_FinalRenderTarget = nullptr;
		Ref<Scene> m_SceneContext = nullptr;
		EditorPickRegistry* m_PickRegistry = nullptr;
		ViewportCamera* m_ViewportCamera = nullptr;
		VulkanResourceFactory* m_VulkanResFactory = nullptr;
		PipelineFactory*  m_PipelineFactory = nullptr;


		//ibl
		Ref<ImageBasedLighting> m_IBL = nullptr;

		//graph
		Unique<RenderGraph> m_RenderGraph = nullptr;
	};

}
