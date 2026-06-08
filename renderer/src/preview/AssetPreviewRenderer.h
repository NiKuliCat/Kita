#pragma once

#include "PreviewThumbnailCache.h"
#include "core/Core.h"
#include "render/VulkanMaterial.h"
#include "render/pass/RenderDataStruct.h"

namespace Kita {

	class PipelineFactory;
	class PreviewSpherePass;
	class SceneBindings;
	class VulkanContext;
	class VulkanGeometry;
	class VulkanGraphicsPipeline;
	class VulkanImage;
	class VulkanRenderTarget;
	class VulkanResourceFactory;
	class VulkanTexture;

	struct AssetPreviewRequest
	{
		AssetHandle Handle = InvalidAssetHandle;
		AssetPreviewType Type = AssetPreviewType::Texture2D;
		uint32_t Size = 128;
		uint64_t Revision = 0;
	};

	class AssetPreviewRenderer
	{
	public:
		AssetPreviewRenderer(VulkanContext& context, VulkanResourceFactory& resourceFactory);
		~AssetPreviewRenderer();

		AssetPreviewRenderer(const AssetPreviewRenderer&) = delete;
		AssetPreviewRenderer& operator=(const AssetPreviewRenderer&) = delete;

		PreviewThumbnailHandle TryGetCached(const AssetPreviewRequest& request);
		PreviewThumbnailHandle Render(const AssetPreviewRequest& request);
		PreviewThumbnailHandle GetOrRender(const AssetPreviewRequest& request);

		void Invalidate(AssetHandle handle);
		void Clear();

	private:
		struct PreviewTarget
		{
			uint32_t Size = 0;
			Unique<VulkanRenderTarget> RenderTarget = nullptr;
			Unique<PreviewSpherePass> SpherePass = nullptr;
		};

	private:
		PreviewThumbnailKey MakeKey(const AssetPreviewRequest& request) const;
		const VulkanImage* RenderPreviewImage(const AssetPreviewRequest& request);
		const VulkanImage* RenderCubemapSpherePreview(AssetHandle handle, uint32_t size);
		const VulkanImage* RenderMaterialSpherePreview(AssetHandle handle, uint32_t size);
		PreviewTarget& GetOrCreateTarget(uint32_t size);
		bool EnsureSharedResources();
		Ref<VulkanMaterial> GetOrCreateCubemapPreviewMaterial(AssetHandle handle, const Ref<VulkanTexture>& texture);
		const VulkanMaterial::PassRuntime* FindPreviewPass(const VulkanMaterial& material) const;
		VulkanGraphicsPipeline* GetMaterialPreviewPipeline(
			PreviewTarget& target,
			VulkanMaterial& material,
			const VulkanMaterial::PassRuntime& materialPass,
			bool useLegacyDefaults);
		VulkanGraphicsPipeline* GetCubemapPreviewPipeline(PreviewTarget& target, VulkanMaterial& material);

		static ScenePassData BuildPreviewSceneData();

	private:
		VulkanContext* m_Context = nullptr;
		VulkanResourceFactory* m_ResourceFactory = nullptr;
		PreviewThumbnailCache m_ThumbnailCache;
		std::unordered_map<uint32_t, PreviewTarget> m_Targets;
		Unique<SceneBindings> m_SceneBindings = nullptr;
		Unique<PipelineFactory> m_PipelineFactory = nullptr;
		Ref<VulkanGeometry> m_SphereGeometry = nullptr;
		std::unordered_map<AssetHandle, Ref<VulkanMaterial>> m_CubemapPreviewMaterials;
		AssetHandle m_CubemapPreviewShaderLabHandle = InvalidAssetHandle;
	};

}
