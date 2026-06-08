#pragma once

#include <EngineCore.h>
#include <EngineRender.h>

#include "ui/viewport/EditorViewportSurface.h"

namespace Kita {

	// 实时 LookDev 预览使用的简化轨道相机状态。
	struct PreviewOrbitCameraState
	{
		float YawDegrees = -32.0f;
		float PitchDegrees = 18.0f;
		float Distance = 3.0f;
		glm::vec3 Target = glm::vec3(0.0f);
	};

	// 统一承载材质与 cubemap 的实时离屏预览渲染。
	class PreviewSceneRenderer
	{
	public:
		PreviewSceneRenderer(
			VulkanContext& context,
			VulkanResourceFactory& resourceFactory,
			PipelineFactory& pipelineFactory);
		~PreviewSceneRenderer();

		PreviewSceneRenderer(const PreviewSceneRenderer&) = delete;
		PreviewSceneRenderer& operator=(const PreviewSceneRenderer&) = delete;

		void SetIBLSource(const Ref<ImageBasedLighting>& ibl) { m_IBL = ibl; }
		void Invalidate(AssetHandle handle);
		void Clear();

		bool RenderMaterialDefinitionPreview(
			AssetHandle materialDefinitionHandle,
			EditorViewportSurface& surface,
			const PreviewOrbitCameraState& cameraState);

		bool RenderMaterialPreview(
			AssetHandle materialHandle,
			EditorViewportSurface& surface,
			const PreviewOrbitCameraState& cameraState);

		bool RenderCubemapPreview(
			AssetHandle textureHandle,
			EditorViewportSurface& surface,
			const PreviewOrbitCameraState& cameraState);

	private:
		struct OffscreenTargets
		{
			uint32_t Width = 0;
			uint32_t Height = 0;
			Unique<VulkanRenderTarget> GBuffer = nullptr;
			Unique<VulkanRenderTarget> Lighting = nullptr;
		};

	private:
		bool EnsureSharedResources();
		bool EnsureOffscreenTargets(uint32_t width, uint32_t height);

		Ref<VulkanMaterial> BuildTransientCubemapPreviewMaterial(
			AssetHandle textureHandle,
			const Ref<VulkanTexture>& texture);
		Ref<VulkanMaterial> BuildTransientSkyboxMaterial(
			const Ref<VulkanTexture>& cubemapTexture);
		Ref<VulkanMaterial> GetOrCreatePreviewSkyboxMaterial();

		const VulkanMaterial::PassRuntime* FindMaterialPreviewPass(
			const VulkanMaterial& material,
			PassType passType) const;
		bool HasMaterialPass(const VulkanMaterial& material, PassType passType) const;

		VulkanGraphicsPipeline* GetMaterialPipeline(
			const VulkanRenderTargetView& targetView,
			const VulkanGeometry& geometry,
			const VulkanMaterial& material,
			PassType passType);
		VulkanGraphicsPipeline* GetDeferredLightingPipeline(const VulkanRenderTargetView& targetView);
		VulkanGraphicsPipeline* GetTonemapPipeline(const VulkanRenderTargetView& targetView);
		VulkanGraphicsPipeline* GetSkyboxPipeline(
			const VulkanRenderTargetView& targetView,
			const VulkanMaterial& skyboxMaterial);

		bool FillMaterialPipelineRequest(
			PipelineRequest& request,
			const VulkanRenderTargetView& targetView,
			const VulkanGeometry& geometry,
			const VulkanMaterial& material,
			PassType passType) const;
		void ApplyMaterialRenderState(
			PipelineRequest& request,
			const VulkanMaterial::PassRuntime& materialPass,
			bool useLegacyDefaults) const;

		ScenePassData BuildSceneData(
			const PreviewOrbitCameraState& cameraState,
			uint32_t width,
			uint32_t height,
			const RenderPassBeginInfo& beginInfo) const;
		ObjectData BuildPreviewObjectData() const;

		AssetHandle ResolveShaderLabHandle(const std::filesystem::path& relativePath);
		AssetHandle ResolveCubemapPreviewShaderLabHandle();

		static VulkanRenderTarget::CreateInfo BuildGBufferTargetCreateInfo(uint32_t width, uint32_t height);
		static VulkanRenderTarget::CreateInfo BuildLightingTargetCreateInfo(uint32_t width, uint32_t height);

	private:
		VulkanContext* m_Context = nullptr;
		VulkanResourceFactory* m_ResourceFactory = nullptr;
		PipelineFactory* m_PipelineFactory = nullptr;
		Unique<SceneBindings> m_SceneBindings = nullptr;
		Unique<BasePass> m_BasePass = nullptr;
		Unique<DeferredLightingPass> m_DeferredLightingPass = nullptr;
		Unique<ForwardOpaquePass> m_ForwardOpaquePass = nullptr;
		Unique<ToneMappingPass> m_TonemapPass = nullptr;
		Unique<SkyboxPass> m_SkyboxPass = nullptr;
		OffscreenTargets m_OffscreenTargets{};
		Ref<VulkanGeometry> m_SphereGeometry = nullptr;
		Ref<ImageBasedLighting> m_IBL = nullptr;
		Ref<VulkanMaterial> m_PreviewSkyboxMaterial = nullptr;
		Ref<VulkanTexture> m_DefaultSkyboxTexture = nullptr;
		Ref<VulkanShader> m_DeferredLightingVertexShader = nullptr;
		Ref<VulkanShader> m_DeferredLightingFragmentShader = nullptr;
		Ref<VulkanShader> m_TonemapVertexShader = nullptr;
		Ref<VulkanShader> m_TonemapFragmentShader = nullptr;
		AssetHandle m_CubemapPreviewShaderLabHandle = InvalidAssetHandle;
		std::unordered_map<AssetHandle, Ref<VulkanMaterial>> m_CubemapPreviewMaterials;
	};

}
