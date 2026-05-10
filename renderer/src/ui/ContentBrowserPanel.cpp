#include "renderer_pch.h"
#include "ContentBrowserPanel.h"
#include "SvgIconAtlas.h"

#include "imgui.h"
#include <imgui_internal.h>

#include <algorithm>

namespace Kita {
	namespace
	{
		bool IsSupportedAssetPath(const std::filesystem::path& path)
		{
			std::string extension = path.extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

			return extension == ".mat"
				|| extension == ".glsl"
				|| extension == ".slang"
				|| extension == ".vert"
				|| extension == ".frag"
				|| extension == ".png"
				|| extension == ".jpg"
				|| extension == ".jpeg"
				|| extension == ".tga"
				|| extension == ".bmp"
				|| extension == ".fbx"
				|| extension == ".obj"
				|| extension == ".dae"
				|| extension == ".gltf"
				|| extension == ".glb";
		}
	}

	const ImU32 separatorColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.055f, 0.056f, 0.060f, 1.0f));
	const ImU32  searchColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.102f, 0.102f, 0.102f, 1.0f));
	const ImU32  tabColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.124f, 0.124f, 0.124f, 1.0f));
	const ImU32  tileHoverBgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.270f, 0.220f, 0.070f, 0.55f));
	const ImU32  tileHoverBorderColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.980f, 0.800f, 0.180f, 1.0f));
	const ImU32  previewBgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.165f, 0.165f, 0.170f, 1.0f));
	const ImU32  previewBorderColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.235f, 0.235f, 0.245f, 1.0f));
	constexpr float kTileMinSize = 72.0f;
	constexpr float kTileMaxSize = 160.0f;
	constexpr float kTileInnerPadding = 12.0f;
	constexpr float kTileTopPadding = 5.0f;
	constexpr float kTileIconTextGap = 2.0f;
	constexpr float kTileBottomPadding = 5.0f;
	constexpr float kPreviewFillRatio = 0.64f;
	constexpr float kPreviewMinSize = 40.0f;
	constexpr float kTooltipPreviewMaxSize = 256.0f;
	constexpr float kPreviewCornerRounding = 6.0f;

	ContentBrowserPanel::ContentBrowserPanel(const std::filesystem::path& contentPath)
	{
		SetContentRoot(contentPath);
	}

	void ContentBrowserPanel::SetContentRoot(const std::filesystem::path& contentRoot)
	{
		m_ContentRoot = contentRoot;
		m_SelectedDirectory = contentRoot;
	}

	void ContentBrowserPanel::SetToolbarHeight(float toolbarHeight)
	{
		m_ToolbarHeight = toolbarHeight < 0.0f ? 0.0f : toolbarHeight;
	}

	void ContentBrowserPanel::OnImGuiRender()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGuiWindowClass viewportWindowClass{};
		viewportWindowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
		ImGui::SetNextWindowClass(&viewportWindowClass);

		ImGui::Begin("Content");

		if (m_ContentRoot.empty() || !std::filesystem::exists(m_ContentRoot) || !std::filesystem::is_directory(m_ContentRoot))
		{
			ImGui::TextUnformatted("Asset browser root is invalid.");
			ImGui::End();
			ImGui::PopStyleVar();
			return;
		}

		if (m_SelectedDirectory.empty() || !std::filesystem::exists(m_SelectedDirectory))
		{
			m_SelectedDirectory = m_ContentRoot;
		}

		DrawToolbar();

		ImGui::PushStyleColor(ImGuiCol_Separator, separatorColor);
		ImGui::PushStyleColor(ImGuiCol_SeparatorHovered, separatorColor);
		ImGui::PushStyleColor(ImGuiCol_SeparatorActive, separatorColor);
		ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, separatorColor);
		ImGui::PushStyleColor(ImGuiCol_TableBorderLight, separatorColor);
		ImGui::Separator();


		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));
		ImGui::BeginChild("ContentBrowserSplit", ImVec2(0.0f, 0.0f), false);

		if (ImGui::BeginTable("##ContentBrowserTable", 2, ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("Folders", ImGuiTableColumnFlags_WidthFixed, 280.0f);
			ImGui::TableSetupColumn("Contents", ImGuiTableColumnFlags_WidthStretch);
			ImGui::TableNextRow();

			ImGui::TableSetColumnIndex(0);
			ImGui::BeginChild("##ContentFolders", ImVec2(0.0f, 0.0f), false);
			DrawDirectoryTree(m_ContentRoot);
			ImGui::EndChild();

			ImGui::TableSetColumnIndex(1);
			ImGui::BeginChild("##ContentEntries", ImVec2(0.0f, 0.0f), false);
			DrawDirectoryContents();
			ImGui::EndChild();

			ImGui::EndTable();
		}
		ImGui::PopStyleColor(5);
		ImGui::EndChild();
		ImGui::PopStyleVar(2);

		ImGui::End();

		ImGui::PopStyleVar();
	}

	void ContentBrowserPanel::DrawToolbar()
	{
		const float toolbarHeight = m_ToolbarHeight;
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::BeginChild("##ContentToolbar", ImVec2(0.0f, toolbarHeight), false, ImGuiWindowFlags_NoScrollbar);

		ImGuiStyle& style = ImGui::GetStyle();
		const float horizontalPadding = 6.0f;
		const float rightPadding = 5.0f;
		const float verticalPadding = ImClamp(toolbarHeight * 0.08f, 2.0f, 6.0f);
		const float targetInputHeight = ImMax(1.0f, toolbarHeight - verticalPadding * 2.0f);
		const float framePaddingY = ImMax(0.0f, (targetInputHeight - ImGui::GetFontSize()) * 0.5f);

		const float contentWidth = ImGui::GetContentRegionAvail().x;
		const float minSearchWidth = 120.0f;
		const float preferredSearchWidth = 300.0f;
		const float maxSearchWidth = ImMax(1.0f, contentWidth - horizontalPadding - rightPadding);
		const float clampedMinSearchWidth = ImMin(minSearchWidth, maxSearchWidth);
		const float searchWidth = ImClamp(preferredSearchWidth, clampedMinSearchWidth, maxSearchWidth);
		const float searchPosX = ImMax(horizontalPadding, contentWidth - rightPadding - searchWidth);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, framePaddingY));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, searchColor);

		const float inputHeight = ImGui::GetFrameHeight();
		const float actualToolbarHeight = ImGui::GetWindowSize().y;
		const float visualCenterOffsetY = 1.0f;
		const float centeredY = IM_TRUNC((actualToolbarHeight - inputHeight) * 0.5f + 0.5f) + visualCenterOffsetY;
		const float cursorPosY = ImClamp(centeredY, 0.0f, ImMax(0.0f, actualToolbarHeight - inputHeight));
		ImGui::SetCursorPosY(cursorPosY);
		ImGui::SetCursorPosX(searchPosX);
		ImGui::SetNextItemWidth(searchWidth);
		ImGui::InputTextWithHint("##ContentSearch", "Search...", m_SearchBuffer.data(), m_SearchBuffer.size());

		ImGui::SetCursorPosX(horizontalPadding);
		ImGui::SetCursorPosY(cursorPosY);
		ImGui::SetNextItemWidth(140.0f);
		ImGui::SliderFloat("##ContentTileSize", &m_TileSize, kTileMinSize, kTileMaxSize, "Size %.0f");
		ImGui::PopStyleVar();
		ImGui::PopStyleColor();
		ImGui::EndChild();
		ImGui::PopStyleVar();
	}

	void ContentBrowserPanel::DrawDirectoryTree(const std::filesystem::path& directory)
	{
		const bool isSelected = std::filesystem::equivalent(directory, m_SelectedDirectory);
		const ImGuiTreeNodeFlags flags =
			ImGuiTreeNodeFlags_OpenOnArrow |
			ImGuiTreeNodeFlags_OpenOnDoubleClick |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_DefaultOpen |
			(isSelected ? ImGuiTreeNodeFlags_Selected : 0);

		const std::string label = GetDisplayName(directory);
		const bool isOpen = ImGui::TreeNodeEx(directory.string().c_str(), flags, "%s", label.c_str());

		if (ImGui::IsItemClicked())
		{
			m_SelectedDirectory = directory;
		}

		if (!isOpen)
		{
			return;
		}

		std::vector<std::filesystem::directory_entry> childDirectories;
		for (const auto& entry : std::filesystem::directory_iterator(directory))
		{
			if (entry.is_directory())
			{
				childDirectories.push_back(entry);
			}
		}

		std::sort(childDirectories.begin(), childDirectories.end(),
			[](const auto& left, const auto& right)
			{
				return left.path().filename().string() < right.path().filename().string();
			});

		for (const auto& entry : childDirectories)
		{
			DrawDirectoryTree(entry.path());
		}

		ImGui::TreePop();
	}

	void ContentBrowserPanel::DrawDirectoryContents()
	{
		ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, tabColor);
		ImGui::BeginChild("##ContentBreadcrumbBar", ImVec2(0.0f, 20.0f), false, ImGuiWindowFlags_NoScrollbar);
		DrawBreadcrumb();
		ImGui::EndChild();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8.0f);

		std::vector<ContentEntryInfo> entries;
		for (const auto& entry : std::filesystem::directory_iterator(m_SelectedDirectory))
		{
			if (!ShouldDisplayEntry(entry))
			{
				continue;
			}

			entries.push_back(BuildEntryInfo(entry));
		}

		std::sort(entries.begin(), entries.end(),
			[](const auto& left, const auto& right)
			{
				if (left.entry.is_directory() != right.entry.is_directory())
				{
					return left.entry.is_directory() > right.entry.is_directory();
				}

				return left.entry.path().filename().string() < right.entry.path().filename().string();
			});

		const float panelWidth = ImGui::GetContentRegionAvail().x;
		const int columnCount = std::max(1, static_cast<int>(panelWidth / m_TileSize));

		if (ImGui::BeginTable("##ContentEntryGrid", columnCount, ImGuiTableFlags_SizingFixedFit))
		{
			for (const auto& entry : entries)
			{
				ImGui::TableNextColumn();
				ImGui::PushID(entry.entry.path().string().c_str());

				const bool isDirectory = entry.entry.is_directory();
				const std::string filename = entry.entry.path().filename().string();
				const std::string stem = entry.entry.path().stem().string();
				const std::string name = isDirectory || stem.empty() ? filename : stem;
				const char* icon = GetEntryIcon(entry);
				ImFont* font = ImGui::GetFont();
				const ThumbnailCache::ThumbnailHandle thumbnail = GetEntryThumbnail(entry);
				const SvgIconAtlas::IconHandle atlasIcon = GetEntryAtlasIcon(entry, isDirectory);
				const EntryTileLayout layout = BuildEntryTileLayout();
				ImGui::InvisibleButton("##EntryItem", ImVec2(layout.ItemWidth, layout.ItemHeight));
				const bool clicked = ImGui::IsItemClicked(ImGuiMouseButton_Left);
				const bool isHovered = ImGui::IsItemHovered();

				const ImVec2 itemMin = ImGui::GetItemRectMin();
				const ImVec2 itemMax = ImGui::GetItemRectMax();
				DrawEntryVisual(entry, layout, itemMin, itemMax, isHovered, font, icon, thumbnail, atlasIcon);

				const ImVec2 labelMin(itemMin.x, itemMin.y + layout.TopPadding + layout.ContentHeight + kTileIconTextGap);
				const ImVec2 labelMax(itemMax.x, itemMax.y - layout.BottomPadding);
				const ImVec2 labelSize = ImGui::CalcTextSize(name.c_str());
				ImGui::RenderTextClipped(labelMin, labelMax, name.c_str(), nullptr, &labelSize, ImVec2(0.5f, 0.5f));

				if (clicked && isDirectory)
				{
					m_SelectedDirectory = entry.entry.path();
				}

				if (isDirectory && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					m_SelectedDirectory = entry.entry.path();
				}

				if (ImGui::IsItemHovered() && !isDirectory)
				{
					DrawEntryTooltip(entry, filename, thumbnail);
				}

				ImGui::PopID();
			}

			ImGui::EndTable();
		}
	}

	ThumbnailCache::ThumbnailHandle ContentBrowserPanel::GetEntryThumbnail(const ContentEntryInfo& entryInfo)
	{
		if (!m_ThumbnailCache || entryInfo.entry.is_directory() || !entryInfo.isAsset || entryInfo.type != AssetType::Texture)
		{
			return {};
		}

		return m_ThumbnailCache->GetOrCreate(entryInfo.handle, entryInfo.type);
	}

	SvgIconAtlas::IconHandle ContentBrowserPanel::GetEntryAtlasIcon(const ContentEntryInfo& entryInfo, bool isDirectory) const
	{
		if (!m_IconAtlas || !m_IconAtlas->IsLoaded())
		{
			return {};
		}

		if (isDirectory)
		{
			return m_IconAtlas->FindIcon("icons8-folder-100");
		}

		return m_IconAtlas->FindIcon("icons8-file-100");
	}

	ContentBrowserPanel::EntryTileLayout ContentBrowserPanel::BuildEntryTileLayout() const
	{
		EntryTileLayout layout{};
		layout.ItemWidth = ImMax(1.0f, m_TileSize - kTileInnerPadding);
		layout.TopPadding = kTileTopPadding;
		layout.ContentHeight = GetPreviewSize();
		layout.BottomPadding = kTileBottomPadding;
		layout.ItemHeight = layout.TopPadding + layout.ContentHeight + kTileIconTextGap + ImGui::GetTextLineHeight() + layout.BottomPadding;
		return layout;
	}

	float ContentBrowserPanel::GetPreviewSize() const
	{
		return ImClamp((m_TileSize - kTileInnerPadding) * kPreviewFillRatio, kPreviewMinSize, m_TileSize);
	}

	void ContentBrowserPanel::DrawEntryVisual(
		const ContentEntryInfo& entryInfo,
		const EntryTileLayout& layout,
		const ImVec2& itemMin,
		const ImVec2& itemMax,
		bool isHovered,
		ImFont* font,
		const char* icon,
		ThumbnailCache::ThumbnailHandle thumbnail,
		SvgIconAtlas::IconHandle atlasIcon) const
	{
		const float previewSize = layout.ContentHeight;
		const ImVec2 previewMin(
			itemMin.x + (itemMax.x - itemMin.x - previewSize) * 0.5f,
			itemMin.y + layout.TopPadding);
		const ImVec2 previewMax(previewMin.x + previewSize, previewMin.y + previewSize);
		ImDrawList* drawList = ImGui::GetWindowDrawList();

		if (isHovered)
		{
			drawList->AddRectFilled(itemMin, itemMax, tileHoverBgColor, kPreviewCornerRounding);
			drawList->AddRect(itemMin, itemMax, tileHoverBorderColor, kPreviewCornerRounding, 0, 1.5f);
		}

		drawList->AddRectFilled(previewMin, previewMax, previewBgColor, kPreviewCornerRounding);
		drawList->AddRect(
			previewMin,
			previewMax,
			previewBorderColor,
			kPreviewCornerRounding,
			0,
			1.0f);

		if (thumbnail.IsValid())
		{
			drawList->AddImage(
				thumbnail.TextureID,
				previewMin,
				previewMax,
				ImVec2(0.0f, 0.0f),
				ImVec2(1.0f, 1.0f));
			return;
		}

		if (atlasIcon.IsValid())
		{
			drawList->AddImage(
				atlasIcon.TextureID,
				previewMin,
				previewMax,
				atlasIcon.UV0,
				atlasIcon.UV1);
			return;
		}

		// 回退：使用 TTF 字体图标
		const float iconFontSize = ImMax(18.0f, previewSize * 0.7f);
		unsigned int iconCodepoint = 0;
		ImTextCharFromUtf8(&iconCodepoint, icon, nullptr);
		const ImFontGlyph* glyph = font->FindGlyph(static_cast<ImWchar>(iconCodepoint));
		const float glyphScale = iconFontSize / font->FontSize;
		const ImVec2 iconSize = font->CalcTextSizeA(iconFontSize, FLT_MAX, 0.0f, icon);
		const ImVec2 visibleIconSize = glyph
			? ImVec2((glyph->X1 - glyph->X0) * glyphScale, (glyph->Y1 - glyph->Y0) * glyphScale)
			: iconSize;
		const ImVec2 glyphOffset = glyph
			? ImVec2(glyph->X0 * glyphScale, glyph->Y0 * glyphScale)
			: ImVec2(0.0f, 0.0f);
		const ImVec2 iconPos(
			previewMin.x + (previewSize - visibleIconSize.x) * 0.5f - glyphOffset.x,
			previewMin.y + (previewSize - visibleIconSize.y) * 0.5f - glyphOffset.y);
		drawList->AddText(font, iconFontSize, iconPos, ImGui::GetColorU32(ImGuiCol_Text), icon);
	}

	void ContentBrowserPanel::DrawEntryTooltip(
		const ContentEntryInfo& entryInfo,
		const std::string& filename,
		ThumbnailCache::ThumbnailHandle thumbnail) const
	{
		ImGui::BeginTooltip();

		if (thumbnail.IsValid())
		{
			const float width = static_cast<float>(thumbnail.Width);
			const float height = static_cast<float>(thumbnail.Height);
			const float maxDimension = ImMax(width, height);
			if (maxDimension > 0.0f)
			{
				const float scale = ImMin(1.0f, kTooltipPreviewMaxSize / maxDimension);
				const ImVec2 previewSize(width * scale, height * scale);
				ImGui::Image(thumbnail.TextureID, previewSize, ImVec2(0.0f, 0.0f), ImVec2(1.0f, 1.0f));
				ImGui::Separator();
			}
		}

		ImGui::Text("File: %s", filename.c_str());
		if (entryInfo.isAsset)
		{
			ImGui::Separator();
			ImGui::Text("Asset Type: %s", GetAssetTypeLabel(entryInfo.type));
			ImGui::Text("Handle: %llu", static_cast<unsigned long long>(entryInfo.handle));
			if (thumbnail.IsValid() && thumbnail.Width > 0 && thumbnail.Height > 0)
			{
				ImGui::Text("Size: %u x %u", thumbnail.Width, thumbnail.Height);
			}
		}
		else
		{
			ImGui::Separator();
			ImGui::TextUnformatted("Non-asset file");
		}
		ImGui::EndTooltip();
	}

	void ContentBrowserPanel::DrawBreadcrumb()
	{
		ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 0.0f));

		const std::string rootLabel = GetDisplayName(m_ContentRoot);
		if (ImGui::Button(rootLabel.c_str()))
		{
			m_SelectedDirectory = m_ContentRoot;
		}

		if (std::filesystem::equivalent(m_SelectedDirectory, m_ContentRoot))
		{
			ImGui::PopStyleVar(2);
			return;
		}

		std::filesystem::path relativePath;
		try
		{
			relativePath = std::filesystem::relative(m_SelectedDirectory, m_ContentRoot);
		}
		catch (...)
		{
			ImGui::PopStyleVar(2);
			return;
		}

		std::filesystem::path cumulativePath = m_ContentRoot;
		for (const auto& part : relativePath)
		{
			cumulativePath /= part;
			ImGui::SameLine();
			ImGui::TextUnformatted("/");
			ImGui::SameLine();
			if (ImGui::Button(part.string().c_str()))
			{
				m_SelectedDirectory = cumulativePath;
			}
		}

		ImGui::PopStyleVar(2);
	}

	std::string ContentBrowserPanel::GetDisplayName(const std::filesystem::path& path) const
	{
		if (std::filesystem::equivalent(path, m_ContentRoot))
		{
			return "Assets";
		}

		const std::string filename = path.filename().string();
		return filename.empty() ? path.string() : filename;
	}

	bool ContentBrowserPanel::ShouldDisplayEntry(const std::filesystem::directory_entry& entry) const
	{
		if (entry.path().extension() == ".meta")
		{
			return false;
		}

		const std::string filename = entry.path().filename().string();
		if (filename == "imgui.ini")
		{
			return false;
		}

		if (entry.path().extension() == ".kitaproj")
		{
			return false;
		}

		return true;
	}

	ContentBrowserPanel::ContentEntryInfo ContentBrowserPanel::BuildEntryInfo(const std::filesystem::directory_entry& entry)
	{
		ContentEntryInfo entryInfo{};
		entryInfo.entry = entry;

		if (entry.is_directory())
		{
			return entryInfo;
		}

		const auto& path = entry.path();
		if (!IsSupportedAssetPath(path))
		{
			return entryInfo;
		}

		auto& assetManager = AssetManager::GetInstance();
		AssetHandle handle = assetManager.GetHandleByPath(path);
		if (handle == InvalidAssetHandle)
		{
			handle = assetManager.ImportAsset(path);
		}

		if (handle == InvalidAssetHandle)
		{
			return entryInfo;
		}

		entryInfo.isAsset = true;
		entryInfo.handle = handle;

		if (const AssetMetadata* metadata = assetManager.GetMetadata(handle))
		{
			entryInfo.type = metadata->type;
		}

		return entryInfo;
	}

	const char* ContentBrowserPanel::GetEntryIcon(const ContentEntryInfo& entryInfo) const
	{
		if (entryInfo.entry.is_directory())
		{
			return ICON_FON_FOLDER;
		}

		switch (entryInfo.type)
		{
		case AssetType::Texture:
			return ICON_FON_PICTURE;
		case AssetType::Mesh:
			return ICON_FON_CUBE;
		case AssetType::Shader:
			return ICON_FON_DOC_TEXT;
		default:
			return ICON_FON_DOC_TEXT;
		}
	}

	const char* ContentBrowserPanel::GetAssetTypeLabel(AssetType type) const
	{
		switch (type)
		{
		case AssetType::Material:
			return "Material";
		case AssetType::Shader:
			return "Shader";
		case AssetType::Texture:
			return "Texture";
		case AssetType::Mesh:
			return "Mesh";
		default:
			return "None";
		}
	}
}
