#include "renderer_pch.h"
#include "ContentBrowserPanel.h"

#include "imgui.h"
#include <imgui_internal.h>

#include <algorithm>

namespace Kita {

	const ImU32 separatorColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.055f, 0.056f, 0.060f, 1.0f));
	const ImU32  searchColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.102f, 0.102f, 0.102f, 1.0f));
	const ImU32  tabColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.124f, 0.124f, 0.124f, 1.0f));

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
			ImGui::TextUnformatted("Content root is invalid.");
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

		const float minSearchWidth = 120.0f;
		const float preferredSearchWidth = 300.0f;
		const float contentWidth = ImGui::GetContentRegionAvail().x;
		const float maxSearchWidth = ImMax(1.0f, contentWidth - horizontalPadding - rightPadding);
		const float clampedMinSearchWidth = ImMin(minSearchWidth, maxSearchWidth);
		const float searchWidth = ImClamp(preferredSearchWidth, clampedMinSearchWidth, maxSearchWidth);
		const float searchPosX = ImMax(horizontalPadding, contentWidth - rightPadding - searchWidth);

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(style.FramePadding.x, framePaddingY));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, searchColor);

		const float inputHeight = ImGui::GetFrameHeight();
		const float actualToolbarHeight = ImGui::GetWindowSize().y;
		const float visualCenterOffsetY = 1.0f;
		const float cursorPosY = ImClamp(
			IM_TRUNC((actualToolbarHeight - inputHeight) * 0.5f + 0.5f) + visualCenterOffsetY,
			0.0f,
			ImMax(0.0f, actualToolbarHeight - inputHeight));
		ImGui::SetCursorPosY(cursorPosY);
		ImGui::SetCursorPosX(searchPosX);
		ImGui::SetNextItemWidth(searchWidth);
		ImGui::InputTextWithHint("##ContentSearch", "Search...", m_SearchBuffer.data(), m_SearchBuffer.size());
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

		std::vector<std::filesystem::directory_entry> entries;
		for (const auto& entry : std::filesystem::directory_iterator(m_SelectedDirectory))
		{
			entries.push_back(entry);
		}

		std::sort(entries.begin(), entries.end(),
			[](const auto& left, const auto& right)
			{
				if (left.is_directory() != right.is_directory())
				{
					return left.is_directory() > right.is_directory();
				}

				return left.path().filename().string() < right.path().filename().string();
			});

		const float tileSize = 92.0f;
		const float panelWidth = ImGui::GetContentRegionAvail().x;
		const int columnCount = std::max(1, static_cast<int>(panelWidth / tileSize));

		if (ImGui::BeginTable("##ContentEntryGrid", columnCount, ImGuiTableFlags_SizingFixedFit))
		{
			for (const auto& entry : entries)
			{
				ImGui::TableNextColumn();
				ImGui::PushID(entry.path().string().c_str());

				const bool isDirectory = entry.is_directory();
				const std::string filename = entry.path().filename().string();
				const std::string stem = entry.path().stem().string();
				const std::string name = isDirectory || stem.empty() ? filename : stem;
				const float itemWidth = tileSize - 12.0f;
				const float topPadding = 5.0f;
				const float iconTextGap = 2.0f;
				const float bottomPadding = 5.0f;
				const float iconFontSize = 32.0f;
				const char* icon = isDirectory ? ICON_FON_FOLDER : ICON_FON_DOC_TEXT;
				ImFont* font = ImGui::GetFont();
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
				const float itemHeight = topPadding + visibleIconSize.y + iconTextGap + ImGui::GetTextLineHeight() + bottomPadding;
				bool clicked = ImGui::Selectable("##EntryItem", false, 0, ImVec2(itemWidth, itemHeight));

				const ImVec2 itemMin = ImGui::GetItemRectMin();
				const ImVec2 itemMax = ImGui::GetItemRectMax();
				const ImVec2 iconPos(
					itemMin.x + (itemMax.x - itemMin.x - visibleIconSize.x) * 0.5f - glyphOffset.x,
					itemMin.y + topPadding - glyphOffset.y);
				ImGui::GetWindowDrawList()->AddText(font, iconFontSize, iconPos, ImGui::GetColorU32(ImGuiCol_Text), icon);

				const ImVec2 labelMin(itemMin.x, itemMin.y + topPadding + visibleIconSize.y + iconTextGap);
				const ImVec2 labelMax(itemMax.x, itemMax.y - bottomPadding);
				const ImVec2 labelSize = ImGui::CalcTextSize(name.c_str());
				ImGui::RenderTextClipped(labelMin, labelMax, name.c_str(), nullptr, &labelSize, ImVec2(0.5f, 0.5f));

				if (clicked && isDirectory)
				{
					m_SelectedDirectory = entry.path();
				}

				if (isDirectory && ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
				{
					m_SelectedDirectory = entry.path();
				}

				ImGui::PopID();
			}

			ImGui::EndTable();
		}
	}

	void ContentBrowserPanel::DrawBreadcrumb()
	{
		ImGui::SetCursorPos(ImVec2(0.0f, 0.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(8.0f, 0.0f));

		if (ImGui::Button("Content"))
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
			return "Content";
		}

		const std::string filename = path.filename().string();
		return filename.empty() ? path.string() : filename;
	}
}
