#include "renderer_pch.h"
#include "UIAttributeUtil.h"
#include <imgui_internal.h>

namespace Kita {

	namespace
	{
		std::string GetAssetLabel(const AssetMetadata& metadata, UIAttributeUtil::AssetLabelMode labelMode)
		{
			if (labelMode == UIAttributeUtil::AssetLabelMode::RelativePath)
			{
				return metadata.relativePath.generic_string();
			}

			const std::string fileName = metadata.relativePath.filename().string();
			if (!fileName.empty())
			{
				return fileName;
			}

			return metadata.relativePath.generic_string();
		}

		std::string GetCurrentAssetLabel(
			AssetHandle handle,
			UIAttributeUtil::AssetLabelMode labelMode,
			const char* noneLabel)
		{
			if (!Asset::IsValidHandle(handle))
			{
				return noneLabel ? noneLabel : "None";
			}

			if (const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(handle))
			{
				return GetAssetLabel(*metadata, labelMode);
			}

			return noneLabel ? noneLabel : "None";
		}
	}

	UIAttributeUtil::TableStyle UIAttributeUtil::CreateDefaultTableStyle()
	{
		return {};
	}

	bool UIAttributeUtil::BeginPropertyTable(const char* id, const TableStyle& style)
	{
		const ImGuiTableFlags tableFlags =
			ImGuiTableFlags_SizingStretchProp |
			ImGuiTableFlags_BordersInnerH |
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_BordersOuter |
			ImGuiTableFlags_NoSavedSettings;

		ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, style.BorderColor);
		ImGui::PushStyleColor(ImGuiCol_TableBorderLight, style.BorderColor);
		ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(style.BorderColor));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, style.FramePadding);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, style.FrameRounding);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, style.FrameBorderSize);
		ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(0.0f, 0.0f));

		const float tableMinX = ImGui::GetWindowContentRegionMin().x;
		const float tableMaxX = ImGui::GetWindowContentRegionMax().x;
		ImGui::SetCursorPosX(tableMinX);

		const bool open = ImGui::BeginTable(id, 3, tableFlags, ImVec2(tableMaxX - tableMinX, 0.0f));
		if (!open)
		{
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(4);
			return false;
		}

		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, style.LabelColumnWidth);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed, style.ResetColumnWidth);
		return true;
	}

	void UIAttributeUtil::EndPropertyTable()
	{
		ImGui::EndTable();
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(4);
	}

	float UIAttributeUtil::GetContentHeight(const TableStyle& style, float rowHeight)
	{
		const float resolvedRowHeight = rowHeight > 0.0f ? rowHeight : style.RowHeight;
		return resolvedRowHeight - ImGui::GetStyle().CellPadding.y * 2.0f;
	}

	float UIAttributeUtil::GetLabelYOffset(const TableStyle& style, float rowHeight)
	{
		return ImMax(0.0f, (GetContentHeight(style, rowHeight) - ImGui::GetTextLineHeight()) * 0.5f);
	}

	float UIAttributeUtil::GetControlYOffset(const TableStyle& style, float rowHeight, float controlHeight)
	{
		const float resolvedControlHeight = controlHeight > 0.0f ? controlHeight : ImGui::GetFrameHeight();
		return ImMax(0.0f, (GetContentHeight(style, rowHeight) - resolvedControlHeight) * 0.5f);
	}

	void UIAttributeUtil::BeginPropertyRow(const TableStyle& style, float rowHeight)
	{
		const float resolvedRowHeight = rowHeight > 0.0f ? rowHeight : style.RowHeight;
		ImGui::TableNextRow(ImGuiTableRowFlags_None, resolvedRowHeight);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, style.RowBgColor);
	}

	void UIAttributeUtil::DrawPropertyLabelCell(const char* label, const TableStyle& style, float rowHeight)
	{
		ImGui::TableSetColumnIndex(0);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + GetLabelYOffset(style, rowHeight));
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + style.CellInnerPaddingX);
		ImGui::TextUnformatted(label);
	}

	void UIAttributeUtil::PreparePropertyValueCell(const TableStyle& style, float yOffset)
	{
		ImGui::TableSetColumnIndex(1);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yOffset);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + style.CellInnerPaddingX);
	}

	void UIAttributeUtil::PreparePropertyResetCell(const TableStyle& style, float yOffset)
	{
		ImGui::TableSetColumnIndex(2);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yOffset);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + style.ResetCellPaddingX);
	}

	void UIAttributeUtil::DrawEmptyResetCell(const TableStyle& style, float rowHeight)
	{
		PreparePropertyResetCell(style, GetLabelYOffset(style, rowHeight));
		ImGui::Dummy(ImVec2(0.0f, 0.0f));
	}

	bool UIAttributeUtil::DrawResetButtonCell(const char* id, const TableStyle& style, bool enabled, float yOffset)
	{
		const float buttonHeight = ImGui::GetFontSize() + style.FramePadding.y * 2.0f;
		const float resolvedYOffset = yOffset >= 0.0f
			? yOffset
			: GetControlYOffset(style, 0.0f, buttonHeight);
		PreparePropertyResetCell(style, resolvedYOffset);
		ImGui::PushID(id);
		if (!enabled)
		{
			ImGui::BeginDisabled();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		const bool clicked = ImGui::Button("R", ImVec2(style.ResetButtonWidth, 0.0f));
		ImGui::PopStyleVar();

		if (!enabled)
		{
			ImGui::EndDisabled();
		}
		ImGui::PopID();
		return enabled && clicked;
	}

	void UIAttributeUtil::PushInputStyle(const TableStyle& style)
	{
		ImGui::PushStyleColor(ImGuiCol_FrameBg, style.InputFrameBg);
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, style.InputFrameBgHovered);
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, style.InputFrameBgActive);
		ImGui::PushStyleColor(ImGuiCol_Border, style.InputBorderColor);
	}

	void UIAttributeUtil::PopInputStyle()
	{
		ImGui::PopStyleColor(4);
	}

	bool UIAttributeUtil::DrawAssetCombo(
		const char* id,
		AssetHandle& handle,
		const std::vector<AssetMetadata>& assets,
		float width,
		AssetLabelMode labelMode,
		const char* noneLabel,
		bool includeNone)
	{
		bool changed = false;
		const std::string currentLabel = GetCurrentAssetLabel(handle, labelMode, noneLabel);
		ImGui::SetNextItemWidth(width);
		if (ImGui::BeginCombo(id, currentLabel.c_str()))
		{
			if (includeNone)
			{
				const bool noneSelected = !Asset::IsValidHandle(handle);
				if (ImGui::Selectable(noneLabel ? noneLabel : "None", noneSelected))
				{
					handle = InvalidAssetHandle;
					changed = true;
				}

				if (noneSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}

			for (const auto& metadata : assets)
			{
				const std::string optionLabel = GetAssetLabel(metadata, labelMode);
				const bool isSelected = handle == metadata.handle;
				if (ImGui::Selectable(optionLabel.c_str(), isSelected))
				{
					handle = metadata.handle;
					changed = true;
				}

				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}

		return changed;
	}

}
