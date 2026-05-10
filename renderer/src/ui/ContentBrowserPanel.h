#pragma once
#include <EngineCore.h>
#include "ThumbnailCache.h"
#include "SvgIconAtlas.h"
namespace Kita {

	class ContentBrowserPanel {

	public:
		ContentBrowserPanel() = default;
		ContentBrowserPanel(const std::filesystem::path& contentPath);


		void SetContentRoot(const std::filesystem::path& contentRoot);
		void SetToolbarHeight(float toolbarHeight);
		void SetThumbnailCache(ThumbnailCache* thumbnailCache) { m_ThumbnailCache = thumbnailCache; }
		void SetIconAtlas(SvgIconAtlas* iconAtlas) { m_IconAtlas = iconAtlas; }
		void OnImGuiRender();


	private:
		struct ContentEntryInfo
		{
			std::filesystem::directory_entry entry;
			bool isAsset = false;
			AssetHandle handle = InvalidAssetHandle;
			AssetType type = AssetType::None;
		};

		struct EntryTileLayout
		{
			float ItemWidth = 0.0f;
			float ItemHeight = 0.0f;
			float TopPadding = 0.0f;
			float ContentHeight = 0.0f;
			float BottomPadding = 0.0f;
		};

		void DrawToolbar();
		void DrawDirectoryTree(const std::filesystem::path& directory);
		void DrawDirectoryContents();
		void DrawBreadcrumb();
		ThumbnailCache::ThumbnailHandle GetEntryThumbnail(const ContentEntryInfo& entryInfo);
		SvgIconAtlas::IconHandle GetEntryAtlasIcon(const ContentEntryInfo& entryInfo, bool isDirectory) const;
		EntryTileLayout BuildEntryTileLayout() const;
		float GetPreviewSize() const;
		void DrawEntryVisual(
			const ContentEntryInfo& entryInfo,
			const EntryTileLayout& layout,
			const ImVec2& itemMin,
			const ImVec2& itemMax,
			bool isHovered,
			ImFont* font,
			const char* icon,
			ThumbnailCache::ThumbnailHandle thumbnail,
			SvgIconAtlas::IconHandle atlasIcon) const;
		void DrawEntryTooltip(
			const ContentEntryInfo& entryInfo,
			const std::string& filename,
			ThumbnailCache::ThumbnailHandle thumbnail) const;
		std::string GetDisplayName(const std::filesystem::path& path) const;
		bool ShouldDisplayEntry(const std::filesystem::directory_entry& entry) const;
		ContentEntryInfo BuildEntryInfo(const std::filesystem::directory_entry& entry);
		const char* GetEntryIcon(const ContentEntryInfo& entryInfo) const;
		const char* GetAssetTypeLabel(AssetType type) const;
	private:
		std::filesystem::path m_ContentRoot;
		std::filesystem::path m_SelectedDirectory;
		std::array<char, 256> m_SearchBuffer{};
		float m_ToolbarHeight = 24.0f;
		float m_TileSize = 92.0f;

		ThumbnailCache* m_ThumbnailCache = nullptr;
		SvgIconAtlas* m_IconAtlas = nullptr;
	};
}
