#pragma once
#include <EngineCore.h>
#include "ThumbnailCache.h"
#include "SvgIconAtlas.h"
#include "EditorSelectionContext.h"
namespace Kita {

	struct ContentEntryInfo
	{
		std::filesystem::path AbsolutePath;
		std::filesystem::path RelativePath;

		std::string FileName;
		std::string DisplayName;

		bool IsDirectory = false;
		bool IsAsset = false;

		AssetHandle Handle = InvalidAssetHandle;
		AssetType Type = AssetType::None;
	};

	struct DirectoryCache
	{
		std::filesystem::path Directory;
		std::vector<ContentEntryInfo> Entries;
		bool Valid = false;
	};

	class ContentBrowserPanel {

	public:
		ContentBrowserPanel() = default;
		ContentBrowserPanel(const std::filesystem::path& contentPath,const Ref<EditorSelectionContext>& selectionContext);


		void SetContentRoot(const std::filesystem::path& contentRoot);
		void SetEditorSelectionContext(const Ref<EditorSelectionContext>& selectionContext) { m_CurrentSelectionContext = selectionContext; }
		void SetOpenAssetCallback(std::function<void(AssetHandle)> callback) { m_OpenAssetCallback = std::move(callback); }
		void SetToolbarHeight(float toolbarHeight);
		void SetThumbnailCache(ThumbnailCache* thumbnailCache) { m_ThumbnailCache = thumbnailCache; }
		void SetIconAtlas(SvgIconAtlas* iconAtlas) { m_IconAtlas = iconAtlas; }
		void RefreshCurrentDirectory();
		void OnImGuiRender();


	private:

		

		struct EntryTileLayout
		{
			float ItemWidth = 0.0f;
			float ItemHeight = 0.0f;
			float TopPadding = 0.0f;
			float ContentHeight = 0.0f;
			float BottomPadding = 0.0f;
		};

		void EnsureCurrentDirectoryCache();
		void RebuildDirectoryCache();

		void DrawToolbar();
		void DrawDirectoryTree(const std::filesystem::path& directory);
		void DrawDirectoryContents();
		void DrawBreadcrumb();
		void SetSelectionAsset(const ContentEntryInfo& entry);
		bool SetSelectedDirectory(const std::filesystem::path& directory);
		bool PassSearchFilter(const ContentEntryInfo& entryInfo) const;
		bool IsEntrySelected(const ContentEntryInfo& entryInfo) const;
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
			bool isSelected,
			ImFont* font,
			const char* icon,
			ThumbnailCache::ThumbnailHandle thumbnail,
			SvgIconAtlas::IconHandle atlasIcon) const;
		void DrawEntryTooltip(
			const ContentEntryInfo& entryInfo,
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

		Ref<EditorSelectionContext> m_CurrentSelectionContext = nullptr;
		std::function<void(AssetHandle)> m_OpenAssetCallback = nullptr;
		DirectoryCache  m_CurrentDirectoryCache;
		ThumbnailCache* m_ThumbnailCache = nullptr;
		SvgIconAtlas* m_IconAtlas = nullptr;
	};
}
