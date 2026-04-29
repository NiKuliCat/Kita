#include "renderer_pch.h"
#include "UIColorPanel.h"

#include "imgui.h"
#include "utils/FileDialogs.h"
#include <imgui_internal.h>

#include <cctype>
#include <cstdio>
#include <cstring>

namespace Kita {

	namespace {

		struct UIColorItem
		{
			ImGuiCol Index;
			const char* Group;
		};

		const UIColorItem s_ColorItems[] = {
			{ ImGuiCol_Text, "Text" },
			{ ImGuiCol_TextDisabled, "Text" },
			{ ImGuiCol_TextLink, "Text" },
			{ ImGuiCol_TextSelectedBg, "Text" },

			{ ImGuiCol_WindowBg, "Windows" },
			{ ImGuiCol_ChildBg, "Windows" },
			{ ImGuiCol_PopupBg, "Windows" },
			{ ImGuiCol_Border, "Windows" },
			{ ImGuiCol_BorderShadow, "Windows" },
			{ ImGuiCol_TitleBg, "Windows" },
			{ ImGuiCol_TitleBgActive, "Windows" },
			{ ImGuiCol_TitleBgCollapsed, "Windows" },
			{ ImGuiCol_MenuBarBg, "Windows" },

			{ ImGuiCol_FrameBg, "Frames" },
			{ ImGuiCol_FrameBgHovered, "Frames" },
			{ ImGuiCol_FrameBgActive, "Frames" },
			{ ImGuiCol_InputTextCursor, "Frames" },
			{ ImGuiCol_CheckMark, "Frames" },
			{ ImGuiCol_SliderGrab, "Frames" },
			{ ImGuiCol_SliderGrabActive, "Frames" },

			{ ImGuiCol_Button, "Buttons" },
			{ ImGuiCol_ButtonHovered, "Buttons" },
			{ ImGuiCol_ButtonActive, "Buttons" },
			{ ImGuiCol_Header, "Buttons" },
			{ ImGuiCol_HeaderHovered, "Buttons" },
			{ ImGuiCol_HeaderActive, "Buttons" },

			{ ImGuiCol_TabHovered, "Tabs And Docking" },
			{ ImGuiCol_Tab, "Tabs And Docking" },
			{ ImGuiCol_TabSelected, "Tabs And Docking" },
			{ ImGuiCol_TabSelectedOverline, "Tabs And Docking" },
			{ ImGuiCol_TabDimmed, "Tabs And Docking" },
			{ ImGuiCol_TabDimmedSelected, "Tabs And Docking" },
			{ ImGuiCol_TabDimmedSelectedOverline, "Tabs And Docking" },
			{ ImGuiCol_DockingPreview, "Tabs And Docking" },
			{ ImGuiCol_DockingEmptyBg, "Tabs And Docking" },

			{ ImGuiCol_TableHeaderBg, "Tables" },
			{ ImGuiCol_TableBorderStrong, "Tables" },
			{ ImGuiCol_TableBorderLight, "Tables" },
			{ ImGuiCol_TableRowBg, "Tables" },
			{ ImGuiCol_TableRowBgAlt, "Tables" },

			{ ImGuiCol_ScrollbarBg, "Navigation And Feedback" },
			{ ImGuiCol_ScrollbarGrab, "Navigation And Feedback" },
			{ ImGuiCol_ScrollbarGrabHovered, "Navigation And Feedback" },
			{ ImGuiCol_ScrollbarGrabActive, "Navigation And Feedback" },
			{ ImGuiCol_Separator, "Navigation And Feedback" },
			{ ImGuiCol_SeparatorHovered, "Navigation And Feedback" },
			{ ImGuiCol_SeparatorActive, "Navigation And Feedback" },
			{ ImGuiCol_ResizeGrip, "Navigation And Feedback" },
			{ ImGuiCol_ResizeGripHovered, "Navigation And Feedback" },
			{ ImGuiCol_ResizeGripActive, "Navigation And Feedback" },
			{ ImGuiCol_TreeLines, "Navigation And Feedback" },
			{ ImGuiCol_DragDropTarget, "Navigation And Feedback" },
			{ ImGuiCol_NavCursor, "Navigation And Feedback" },
			{ ImGuiCol_NavWindowingHighlight, "Navigation And Feedback" },
			{ ImGuiCol_NavWindowingDimBg, "Navigation And Feedback" },
			{ ImGuiCol_ModalWindowDimBg, "Navigation And Feedback" },

			{ ImGuiCol_PlotLines, "Plots" },
			{ ImGuiCol_PlotLinesHovered, "Plots" },
			{ ImGuiCol_PlotHistogram, "Plots" },
			{ ImGuiCol_PlotHistogramHovered, "Plots" }
		};

		const char* s_ColorGroups[] = {
			"Text",
			"Windows",
			"Frames",
			"Buttons",
			"Tabs And Docking",
			"Tables",
			"Navigation And Feedback",
			"Plots",
			"Other"
		};

		const char* FindGroupForColor(ImGuiCol colorIndex)
		{
			for (const UIColorItem& item : s_ColorItems)
			{
				if (item.Index == colorIndex)
				{
					return item.Group;
				}
			}

			return "Other";
		}

		char ToLowerAscii(char value)
		{
			return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
		}

		bool ContainsCaseInsensitive(const char* text, const char* filter)
		{
			if (!filter || filter[0] == '\0')
			{
				return true;
			}

			if (!text)
			{
				return false;
			}

			for (const char* textIt = text; *textIt != '\0'; ++textIt)
			{
				const char* currentText = textIt;
				const char* currentFilter = filter;
				while (*currentText != '\0' &&
					*currentFilter != '\0' &&
					ToLowerAscii(*currentText) == ToLowerAscii(*currentFilter))
				{
					++currentText;
					++currentFilter;
				}

				if (*currentFilter == '\0')
				{
					return true;
				}
			}

			return false;
		}

		int ColorFloatToByte(float value)
		{
			if (value < 0.0f)
			{
				value = 0.0f;
			}
			else if (value > 1.0f)
			{
				value = 1.0f;
			}

			return static_cast<int>(value * 255.0f + 0.5f);
		}

		std::string FormatColorHex(const ImVec4& color)
		{
			char buffer[10]{};
			std::snprintf(
				buffer,
				sizeof(buffer),
				"#%02X%02X%02X%02X",
				ColorFloatToByte(color.x),
				ColorFloatToByte(color.y),
				ColorFloatToByte(color.z),
				ColorFloatToByte(color.w));
			return buffer;
		}

		glm::vec4 ToGlmVec4(const ImVec4& color)
		{
			return { color.x, color.y, color.z, color.w };
		}

		ImVec4 ToImVec4(const glm::vec4& color)
		{
			return { color.x, color.y, color.z, color.w };
		}

		float AbsFloat(float value)
		{
			return value < 0.0f ? -value : value;
		}

		bool IsSameColor(const ImVec4& left, const glm::vec4& right)
		{
			constexpr float epsilon = 0.0001f;
			return AbsFloat(left.x - right.x) < epsilon &&
				AbsFloat(left.y - right.y) < epsilon &&
				AbsFloat(left.z - right.z) < epsilon &&
				AbsFloat(left.w - right.w) < epsilon;
		}

		json SerializeColor(const ImVec4& color)
		{
			return json::array({ color.x, color.y, color.z, color.w });
		}
	}

	void UIColorPanel::OnImGuiRender()
	{
		if (!m_IsOpen)
		{
			return;
		}

		EnsureCapturedColors();

		ImGuiWindowClass viewportWindowClass{};
		viewportWindowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
		ImGui::SetNextWindowClass(&viewportWindowClass);

		if (ImGui::Begin("UI Colors", &m_IsOpen))
		{
			DrawToolbar();
			ImGui::Separator();
			DrawPreview();
			ImGui::Separator();
			DrawColorEditor();
		}

		ImGui::End();
	}

	void UIColorPanel::EnsureCapturedColors()
	{
		if (m_CapturedColors.size() == ImGuiCol_COUNT)
		{
			return;
		}

		CaptureCurrentColors();
	}

	void UIColorPanel::CaptureCurrentColors()
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		m_CapturedColors.resize(ImGuiCol_COUNT);
		for (int i = 0; i < ImGuiCol_COUNT; ++i)
		{
			m_CapturedColors[i] = ToGlmVec4(style.Colors[i]);
		}
	}

	void UIColorPanel::ResetToCapturedColors()
	{
		if (m_CapturedColors.size() != ImGuiCol_COUNT)
		{
			return;
		}

		ImGuiStyle& style = ImGui::GetStyle();
		for (int i = 0; i < ImGuiCol_COUNT; ++i)
		{
			style.Colors[i] = ToImVec4(m_CapturedColors[i]);
		}
	}

	void UIColorPanel::DrawToolbar()
	{
		if (ImGui::Button("Save JSON"))
		{
			SaveCurrentColors();
		}

		ImGui::SameLine();
		if (ImGui::Button("Capture Baseline"))
		{
			CaptureCurrentColors();
			m_LastSaveSucceeded = true;
			m_StatusMessage = "Baseline captured from current colors.";
		}

		ImGui::SameLine();
		ImGui::BeginDisabled(m_CapturedColors.size() != ImGuiCol_COUNT);
		if (ImGui::Button("Reset Baseline"))
		{
			ResetToCapturedColors();
			m_LastSaveSucceeded = true;
			m_StatusMessage = "Colors reset to captured baseline.";
		}
		ImGui::EndDisabled();

		ImGui::SameLine();
		if (ImGui::Button("Dark"))
		{
			ImGui::StyleColorsDark();
			m_LastSaveSucceeded = true;
			m_StatusMessage = "Applied ImGui dark preset.";
		}

		ImGui::SameLine();
		if (ImGui::Button("Light"))
		{
			ImGui::StyleColorsLight();
			m_LastSaveSucceeded = true;
			m_StatusMessage = "Applied ImGui light preset.";
		}

		ImGui::SameLine();
		if (ImGui::Button("Classic"))
		{
			ImGui::StyleColorsClassic();
			m_LastSaveSucceeded = true;
			m_StatusMessage = "Applied ImGui classic preset.";
		}

		ImGui::SetNextItemWidth(-1.0f);
		ImGui::InputTextWithHint("##UIColorSearch", "Search color name or group...", m_SearchBuffer.data(), m_SearchBuffer.size());

		if (!m_StatusMessage.empty())
		{
			const ImVec4 statusColor = m_LastSaveSucceeded
				? ImVec4(0.35f, 0.85f, 0.45f, 1.0f)
				: ImVec4(0.95f, 0.65f, 0.25f, 1.0f);
			ImGui::TextColored(statusColor, "%s", m_StatusMessage.c_str());
		}
	}

	void UIColorPanel::DrawPreview()
	{
		if (!ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen))
		{
			return;
		}

		static bool checkboxValue = true;
		static float sliderValue = 0.45f;
		static int selectedRow = 1;

		ImGui::TextUnformatted("Text");
		ImGui::SameLine();
		ImGui::TextDisabled("Disabled");
		ImGui::SameLine();
		ImGui::TextLink("Link");

		ImGui::Button("Button");
		ImGui::SameLine();
		ImGui::Checkbox("Checkbox", &checkboxValue);
		ImGui::SliderFloat("Slider", &sliderValue, 0.0f, 1.0f);

		if (ImGui::BeginTabBar("##UIColorPreviewTabs"))
		{
			if (ImGui::BeginTabItem("Tab A"))
			{
				ImGui::TextUnformatted("Active tab");
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Tab B"))
			{
				ImGui::TextUnformatted("Second tab");
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		if (ImGui::BeginTable("##UIColorPreviewTable", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_SizingStretchProp))
		{
			ImGui::TableSetupColumn("Name");
			ImGui::TableSetupColumn("State");
			ImGui::TableHeadersRow();
			for (int row = 0; row < 3; ++row)
			{
				ImGui::TableNextRow();
				ImGui::TableSetColumnIndex(0);
				if (ImGui::Selectable(("Preview Row " + std::to_string(row + 1)).c_str(), selectedRow == row, ImGuiSelectableFlags_SpanAllColumns))
				{
					selectedRow = row;
				}
				ImGui::TableSetColumnIndex(1);
				ImGui::TextUnformatted(row == selectedRow ? "Selected" : "Idle");
			}
			ImGui::EndTable();
		}

		ImGui::ProgressBar(sliderValue, ImVec2(-1.0f, 0.0f));
	}

	void UIColorPanel::DrawColorEditor()
	{
		int visibleGroupCount = 0;

		for (const char* groupName : s_ColorGroups)
		{
			if (!HasVisibleColorInGroup(groupName))
			{
				continue;
			}

			++visibleGroupCount;
			if (!ImGui::CollapsingHeader(groupName, ImGuiTreeNodeFlags_DefaultOpen))
			{
				continue;
			}

			ImGui::PushID(groupName);
			const ImGuiTableFlags tableFlags =
				ImGuiTableFlags_RowBg |
				ImGuiTableFlags_BordersInnerV |
				ImGuiTableFlags_Resizable |
				ImGuiTableFlags_SizingStretchProp |
				ImGuiTableFlags_NoSavedSettings;

			if (ImGui::BeginTable("##UIColorTable", 5, tableFlags))
			{
				ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 190.0f);
				ImGui::TableSetupColumn("Edit", ImGuiTableColumnFlags_WidthStretch);
				ImGui::TableSetupColumn("Hex", ImGuiTableColumnFlags_WidthFixed, 92.0f);
				ImGui::TableSetupColumn("RGBA", ImGuiTableColumnFlags_WidthFixed, 190.0f);
				ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed, 64.0f);
				ImGui::TableHeadersRow();

				for (int colorIndex = 0; colorIndex < ImGuiCol_COUNT; ++colorIndex)
				{
					const char* colorGroup = FindGroupForColor(static_cast<ImGuiCol>(colorIndex));
					const char* colorName = ImGui::GetStyleColorName(static_cast<ImGuiCol>(colorIndex));
					if (std::strcmp(colorGroup, groupName) == 0 && PassesFilter(colorName, colorGroup))
					{
						DrawColorRow(colorIndex);
					}
				}

				ImGui::EndTable();
			}
			ImGui::PopID();
		}

		if (visibleGroupCount == 0)
		{
			ImGui::TextUnformatted("No colors match the current search.");
		}
	}

	void UIColorPanel::DrawColorRow(int colorIndex)
	{
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec4& color = style.Colors[colorIndex];
		const char* colorName = ImGui::GetStyleColorName(static_cast<ImGuiCol>(colorIndex));
		const bool canReset = m_CapturedColors.size() == ImGuiCol_COUNT;
		const bool changed = canReset && !IsSameColor(color, m_CapturedColors[colorIndex]);

		ImGui::PushID(colorIndex);
		ImGui::TableNextRow();

		ImGui::TableSetColumnIndex(0);
		ImGui::ColorButton("##Swatch", color, ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(22.0f, 22.0f));
		ImGui::SameLine();
		if (changed)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.82f, 0.28f, 1.0f), "%s", colorName);
		}
		else
		{
			ImGui::TextUnformatted(colorName);
		}

		ImGui::TableSetColumnIndex(1);
		ImGui::SetNextItemWidth(-1.0f);
		ImGui::ColorEdit4(
			"##Color",
			&color.x,
			ImGuiColorEditFlags_DisplayRGB |
			ImGuiColorEditFlags_Float |
			ImGuiColorEditFlags_AlphaBar |
			ImGuiColorEditFlags_AlphaPreviewHalf);

		ImGui::TableSetColumnIndex(2);
		const std::string hexColor = FormatColorHex(color);
		ImGui::TextUnformatted(hexColor.c_str());

		ImGui::TableSetColumnIndex(3);
		ImGui::Text("%.3f, %.3f, %.3f, %.3f", color.x, color.y, color.z, color.w);

		ImGui::TableSetColumnIndex(4);
		ImGui::BeginDisabled(!changed);
		if (ImGui::SmallButton("Reset") && canReset)
		{
			color = ToImVec4(m_CapturedColors[colorIndex]);
		}
		ImGui::EndDisabled();

		ImGui::PopID();
	}

	void UIColorPanel::SaveCurrentColors()
	{
		const std::filesystem::path filepath = FileDialogs::SaveFile(
			L"Kita UI Colors (*.json)\0*.json\0All Files (*.*)\0*.*\0",
			L"json");

		if (filepath.empty())
		{
			m_LastSaveSucceeded = false;
			m_StatusMessage = "Save canceled.";
			return;
		}

		ImGuiStyle& style = ImGui::GetStyle();
		json root;
		root["version"] = 1;
		root["source"] = "Kita UIColorPanel";
		root["imguiVersion"] = ImGui::GetVersion();
		root["colorCount"] = ImGuiCol_COUNT;
		root["colors"] = json::object();
		root["changedColors"] = json::array();

		for (int colorIndex = 0; colorIndex < ImGuiCol_COUNT; ++colorIndex)
		{
			const ImGuiCol imguiColor = static_cast<ImGuiCol>(colorIndex);
			const char* colorName = ImGui::GetStyleColorName(imguiColor);
			const char* groupName = FindGroupForColor(imguiColor);
			const ImVec4& color = style.Colors[colorIndex];

			json colorJson;
			colorJson["index"] = colorIndex;
			colorJson["group"] = groupName;
			colorJson["rgba"] = SerializeColor(color);
			colorJson["hex"] = FormatColorHex(color);
			root["colors"][colorName] = colorJson;

			if (m_CapturedColors.size() == ImGuiCol_COUNT && !IsSameColor(color, m_CapturedColors[colorIndex]))
			{
				root["changedColors"].push_back(colorName);
			}
		}

		if (filepath.has_parent_path())
		{
			std::filesystem::create_directories(filepath.parent_path());
		}

		std::ofstream out(filepath);
		if (!out.is_open())
		{
			m_LastSaveSucceeded = false;
			m_StatusMessage = "Failed to save: " + filepath.string();
			KITA_CLENT_ERROR("Failed to save UI colors: {0}", filepath.string());
			return;
		}

		out << root.dump(4);
		m_LastSavedPath = filepath;
		m_LastSaveSucceeded = true;
		m_StatusMessage = "Saved: " + filepath.string();
		KITA_CLENT_INFO("UI colors saved to {0}", filepath.string());
	}

	bool UIColorPanel::HasVisibleColorInGroup(const char* groupName) const
	{
		for (int colorIndex = 0; colorIndex < ImGuiCol_COUNT; ++colorIndex)
		{
			const ImGuiCol imguiColor = static_cast<ImGuiCol>(colorIndex);
			const char* colorGroup = FindGroupForColor(imguiColor);
			const char* colorName = ImGui::GetStyleColorName(imguiColor);
			if (std::strcmp(colorGroup, groupName) == 0 && PassesFilter(colorName, colorGroup))
			{
				return true;
			}
		}

		return false;
	}

	bool UIColorPanel::PassesFilter(const char* colorName, const char* groupName) const
	{
		const char* filter = m_SearchBuffer.data();
		return ContainsCaseInsensitive(colorName, filter) || ContainsCaseInsensitive(groupName, filter);
	}
}
