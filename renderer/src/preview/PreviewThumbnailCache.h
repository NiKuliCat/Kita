#pragma once

#include "asset/Asset.h"
#include "core/Core.h"

#include "imgui.h"
#include <vulkan/vulkan.h>

namespace Kita {

	class VulkanImage;

	enum class AssetPreviewType : uint8_t
	{
		Texture2D = 0,
		CubemapSphere,
		MaterialSphere
	};

	struct PreviewThumbnailKey
	{
		AssetHandle Handle = InvalidAssetHandle;
		AssetPreviewType Type = AssetPreviewType::Texture2D;
		uint32_t Size = 0;
		uint64_t Revision = 0;

		bool operator==(const PreviewThumbnailKey& other) const
		{
			return Handle == other.Handle &&
				Type == other.Type &&
				Size == other.Size &&
				Revision == other.Revision;
		}
	};

	struct PreviewThumbnailKeyHasher
	{
		size_t operator()(const PreviewThumbnailKey& key) const;
	};

	struct PreviewThumbnailHandle
	{
		ImTextureID TextureID = 0;
		uint32_t Width = 0;
		uint32_t Height = 0;

		bool IsValid() const { return TextureID != 0 && Width > 0 && Height > 0; }
	};

	class PreviewThumbnailCache
	{
	public:
		PreviewThumbnailCache() = default;
		~PreviewThumbnailCache();

		PreviewThumbnailCache(const PreviewThumbnailCache&) = delete;
		PreviewThumbnailCache& operator=(const PreviewThumbnailCache&) = delete;

		// 将已经渲染好的 2D sampled image 注册成 ImGui 可显示的缩略图，并缓存 descriptor 生命周期。
		PreviewThumbnailHandle StoreRenderedImage(
			const PreviewThumbnailKey& key,
			const VulkanImage& image,
			uint32_t width,
			uint32_t height);

		PreviewThumbnailHandle Find(const PreviewThumbnailKey& key);
		void Invalidate(AssetHandle handle);
		void Invalidate(const PreviewThumbnailKey& key);
		void Clear();
		void ProcessPendingReleases();

	private:
		struct CachedThumbnail
		{
			PreviewThumbnailKey Key{};
			PreviewThumbnailHandle Handle{};
			Unique<VulkanImage> Image = nullptr;
		};

		struct RetiredThumbnail
		{
			CachedThumbnail Thumbnail{};
			uint64_t ReleaseAfterFrame = 0;
		};

	private:
		void RetireThumbnail(CachedThumbnail& thumbnail);
		void ReleaseThumbnail(CachedThumbnail& thumbnail);

	private:
		std::unordered_map<PreviewThumbnailKey, CachedThumbnail, PreviewThumbnailKeyHasher> m_Cache;
		std::vector<RetiredThumbnail> m_RetiredThumbnails;
	};

}
