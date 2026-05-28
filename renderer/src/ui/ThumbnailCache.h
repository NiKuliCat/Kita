#pragma once
#include <EngineCore.h>
#include <EngineRender.h>
#include "preview/AssetPreviewRenderer.h"

namespace Kita {

	class ThumbnailCache
	{
	public:
		struct ThumbnailHandle
		{
			ImTextureID TextureID = 0;
			uint32_t Width = 0;
			uint32_t Height = 0;

			bool IsValid() const
			{
				return TextureID != 0;
			}
		};

	public:
		explicit ThumbnailCache(VulkanResourceFactory& resourceFactory);
		~ThumbnailCache();

		ThumbnailCache(const ThumbnailCache&) = delete;
		ThumbnailCache& operator=(const ThumbnailCache&) = delete;

		ThumbnailHandle GetOrCreate(AssetHandle handle, AssetType type, uint32_t preferredSize = 128);

		void SetAssetPreviewRenderer(AssetPreviewRenderer* previewRenderer) { m_AssetPreviewRenderer = previewRenderer; }
		void Invalidate(AssetHandle handle);
		void Clear();

	private:
		struct CachedThumbnail
		{
			AssetHandle Handle = InvalidAssetHandle;
			AssetType Type = AssetType::None;

			Ref<VulkanTexture> Texture = nullptr;
			ImTextureID TextureID = 0;
		};

		struct RetiredThumbnail
		{
			CachedThumbnail Thumbnail{};
			uint64_t ReleaseAfterFrame = 0;
		};

	private:
		ThumbnailHandle GetOrCreateTextureThumbnail(AssetHandle handle, uint32_t preferredSize);
		ThumbnailHandle GetOrCreateCubemapThumbnail(AssetHandle handle, uint32_t size);
		void ReleaseThumbnail(CachedThumbnail& thumbnail);
		void RetireThumbnail(CachedThumbnail& thumbnail);
		void ProcessPendingReleases();


	private:
		VulkanResourceFactory& m_ResourceFactory;
		AssetPreviewRenderer* m_AssetPreviewRenderer = nullptr;
		std::unordered_map<AssetHandle, CachedThumbnail> m_Cache;
		std::vector<RetiredThumbnail> m_RetiredThumbnails;

	};

}
