#pragma once

#include <EngineCore.h>
#include "imgui.h"

namespace Kita {

	class UIAttributeUtil
	{
	public:
		enum class AssetLabelMode
		{
			FileName,
			RelativePath
		};

		struct TableStyle
		{
			float LabelColumnWidth = 130.0f;
			float ResetColumnWidth = 40.0f;
			float CellInnerPaddingX = 12.0f;
			float ValueRightInset = 12.0f;
			float ResetCellPaddingX = 4.0f;
			float ResetButtonWidth = 32.0f;
			float RowHeight = 30.0f;
			float PreviewRowHeight = 80.0f;
			float PreviewTileSize = 70.0f;
			ImVec2 FramePadding = ImVec2(6.0f, 2.0f);
			float FrameRounding = 3.0f;
			float FrameBorderSize = 1.0f;
			ImVec4 InputFrameBg = ImVec4(0.07f, 0.09f, 0.12f, 1.0f);
			ImVec4 InputFrameBgHovered = ImVec4(0.10f, 0.12f, 0.16f, 1.0f);
			ImVec4 InputFrameBgActive = ImVec4(0.12f, 0.15f, 0.20f, 1.0f);
			ImVec4 InputBorderColor = ImVec4(0.20f, 0.24f, 0.30f, 1.0f);
			ImU32 BorderColor = IM_COL32(10, 10, 10, 255);
			ImU32 RowBgColor = IM_COL32(41, 41, 41, 255);
		};

	public:
		static TableStyle CreateDefaultTableStyle();

		static bool BeginPropertyTable(const char* id, const TableStyle& style);
		static void EndPropertyTable();

		static float GetContentHeight(const TableStyle& style, float rowHeight = 0.0f);
		static float GetLabelYOffset(const TableStyle& style, float rowHeight = 0.0f);
		static float GetControlYOffset(const TableStyle& style, float rowHeight = 0.0f, float controlHeight = 0.0f);

		static void BeginPropertyRow(const TableStyle& style, float rowHeight = 0.0f);
		static void DrawPropertyLabelCell(const char* label, const TableStyle& style, float rowHeight = 0.0f);
		static void PreparePropertyValueCell(const TableStyle& style, float yOffset);
		static void PreparePropertyResetCell(const TableStyle& style, float yOffset);
		static void DrawEmptyResetCell(const TableStyle& style, float rowHeight = 0.0f);
		static bool DrawResetButtonCell(const char* id, const TableStyle& style, bool enabled = true, float yOffset = -1.0f);

		static void PushInputStyle(const TableStyle& style);
		static void PopInputStyle();

		static bool DrawAssetCombo(
			const char* id,
			AssetHandle& handle,
			const std::vector<AssetMetadata>& assets,
			float width,
			AssetLabelMode labelMode,
			const char* noneLabel = "None",
			bool includeNone = true);
	};

}
