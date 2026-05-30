#pragma once

#include <EngineCore.h>
#include <EngineRender.h>

#include "EditorGridPass.h"
#include "ViewportCamera.h"
#include "ViewportPickingPass.h"
#include "imgui.h"

namespace Kita {

	class EditorPickRegistry;
	class EditorViewportSurface;

	struct RenderGraphPreviewTexture
	{
		std::string Name;
		ImTextureID TextureID = 0;
		uint32_t Width = 0;
		uint32_t Height = 0;
		VkFormat Format = VK_FORMAT_UNDEFINED;
		RenderGraphResourceID ResourceID = InvalidRenderGraphResourceID;
		uint32_t AttachmentIndex = 0;
		VkImage ImageHandle = VK_NULL_HANDLE;
		bool ActiveThisFrame = false;
	};

	class EditorRenderer
	{
	public:
		EditorRenderer(
			VulkanContext& context,
			const RenderGraphTransientRenderTargetDesc& gbufferTargetDesc,
			const RenderGraphTransientRenderTargetDesc& lightingTargetDesc,
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

		// RenderGraph 管线资源预览列表。
		const std::vector<RenderGraphPreviewTexture>& GetRenderGraphPreviewTextures() const { return m_RenderGraphPreviewTextures; }

	private:
		void InitRenderSceneData(ScenePassData& sceneData);
		void InitGridResources();
		void InitDeferredLightingResources();
		void InitTonemapResources();
		void SyncSkyboxMaterialFromSettings();
		void ResetRenderGraphResourceIDs();
		void UpdateRenderGraphPreviewTextures(const VulkanRenderTarget& finalRt);
		void RegisterRenderGraphPreviewTexture(
			const std::string& name,
			RenderGraphResourceID resourceID,
			uint32_t attachmentIndex,
			const VulkanImage& image);
		void ReleaseRenderGraphPreviewTexture(RenderGraphPreviewTexture& previewTexture);
		void ReleaseRenderGraphPreviewTextures();
		void RemoveInactiveRenderGraphPreviewTextures();

		void BuildRenderGraph(
			EditorViewportSurface& surface,
			VulkanRenderTarget& finalRt,
			VulkanRenderTarget& pickingRt);

		VulkanGraphicsPipeline* GetPipeline(const RenderGraphTransientRenderTargetDesc& targetDesc, Ref<VulkanGeometry>& geometry, Ref<VulkanMaterial>& material);
		VulkanGraphicsPipeline* GetDeferredLightingPipeline(const VulkanRenderTargetView& rt);
		VulkanGraphicsPipeline* GetTonemapPipeline(const VulkanRenderTargetView& rt);
		VulkanGraphicsPipeline* GetGridPipeline(const VulkanRenderTargetView& rt);
		VulkanGraphicsPipeline* GetPickingPipeline(const VulkanRenderTargetView& rt, Ref<VulkanGeometry>& geometry);
		VulkanGraphicsPipeline* GetSkyboxPipeline(const VulkanRenderTargetView& rt);
		void CopyDepthAttachment(
			const VulkanRenderTargetView& sourceRt,
			const VulkanRenderTargetView& targetRt,
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


		VulkanRenderTarget* m_FinalRenderTarget = nullptr;

		Ref<Scene> m_SceneContext = nullptr;
		EditorPickRegistry* m_PickRegistry = nullptr;
		ViewportCamera* m_ViewportCamera = nullptr;
		VulkanResourceFactory* m_VulkanResFactory = nullptr;
		PipelineFactory* m_PipelineFactory = nullptr;

		// ibl
		Ref<ImageBasedLighting> m_IBL = nullptr;

		// graph
		Unique<RenderGraph> m_RenderGraph = nullptr;

		RenderGraphTransientRenderTargetDesc m_LightingTargetDesc{};
		RenderGraphTransientRenderTargetDesc m_GBufferTargetDesc{};
		std::vector<RenderGraphResourceID> m_GBufferColorGraphResourceIDs;
		RenderGraphResourceID m_GBufferDepthGraphResourceID = InvalidRenderGraphResourceID;

		RenderGraphResourceID m_GBufferGraphResourceID = InvalidRenderGraphResourceID;
		RenderGraphResourceID m_LightingGraphResourceID = InvalidRenderGraphResourceID;
		RenderGraphResourceID m_FinalGraphResourceID = InvalidRenderGraphResourceID;
		RenderGraphResourceID m_PickingGraphResourceID = InvalidRenderGraphResourceID;
		std::vector<RenderGraphPreviewTexture> m_RenderGraphPreviewTextures;
	};

}
