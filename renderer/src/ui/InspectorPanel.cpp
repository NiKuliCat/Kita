#include "renderer_pch.h"
#include "InspectorPanel.h"
#include "imgui.h"
#include <imgui_internal.h>

namespace Kita {
	const float inspectorLabelColumn_LeftWidth = 130.0f;
	const float inspectorResetColumn_Width = 40.0f;
	const float inspectorCellInnerPaddingX = 12.0f;
	const float inspectorValueRightInset = inspectorCellInnerPaddingX;
	const float inspectorHeaderHeight = 24.0f;

	const float rowHeight = 30.0f;
	const float itemSpacingX = 12.0f;
	const ImVec2 inspectorControlFramePadding = ImVec2(6.0f, 2.0f);
	const float inspectorControlFrameRounding = 3.0f;
	const float inspectorControlFrameBorderSize = 1.0f;

	const ImU32 inspectorBorderColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.04f, 0.04f, 0.04f, 1.0f));
	const ImU32 tableBgColor_Dark = ImGui::ColorConvertFloat4ToU32(ImVec4(0.16f, 0.16f, 0.16f, 1.0f));

	float InspectorPanel::GetInspectorContentHeight()
	{
		return rowHeight - ImGui::GetStyle().CellPadding.y * 2.0f;
	}

	float InspectorPanel::GetInspectorLabelYOffset()
	{
		return ImMax(0.0f, (GetInspectorContentHeight() - ImGui::GetTextLineHeight()) * 0.5f);
	}

	float InspectorPanel::GetInspectorControlYOffset()
	{
		return ImMax(0.0f, (GetInspectorContentHeight() - ImGui::GetFrameHeight()) * 0.5f);
	}

	void InspectorPanel::DrawItemByType(EditorSelectionItemType type)
	{
		switch (type)
		{
			case EditorSelectionItemType::None:
			{
				return;
			}
			case EditorSelectionItemType::SceneObject:
			{
				DrawSelectedObject(m_SelectionContext->GetSelectionItemHandle().m_SelectionObject);
				return;
			}
			case EditorSelectionItemType::Asset:
			{
				DrawSelectedAsset(m_SelectionContext->GetSelectionItemHandle().m_SelectedAssetHandle);
			}
			default:
			{
				return;
			}
		}
	}

	bool InspectorPanel::BeginPropertyTable(const char* id)
	{
		const ImGuiTableFlags tableFlags =
			ImGuiTableFlags_SizingStretchProp |
			ImGuiTableFlags_BordersInnerH |
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_BordersOuter |
			ImGuiTableFlags_NoSavedSettings;

		ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, inspectorBorderColor);
		ImGui::PushStyleColor(ImGuiCol_TableBorderLight, inspectorBorderColor);
		ImGui::PushStyleColor(ImGuiCol_Border, ImGui::ColorConvertU32ToFloat4(inspectorBorderColor));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, inspectorControlFramePadding);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, inspectorControlFrameRounding);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, inspectorControlFrameBorderSize);
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

		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, inspectorLabelColumn_LeftWidth);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
		ImGui::TableSetupColumn("Reset", ImGuiTableColumnFlags_WidthFixed, inspectorResetColumn_Width);
		return true;
	}

	void InspectorPanel::EndPropertyTable()
	{
		ImGui::EndTable();
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(4);
	}

	void InspectorPanel::BeginPropertyRow(bool& isHighlight)
	{
		ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, tableBgColor_Dark);
		(void)isHighlight;
	}

	void InspectorPanel::DrawPropertyLabelCell(const char* label)
	{
		ImGui::TableSetColumnIndex(0);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + GetInspectorLabelYOffset());
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + inspectorCellInnerPaddingX);
		ImGui::TextUnformatted(label);
	}

	void InspectorPanel::PreparePropertyValueCell(float yOffset)
	{
		ImGui::TableSetColumnIndex(1);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yOffset);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + inspectorCellInnerPaddingX);
	}

	void InspectorPanel::PreparePropertyResetCell(float yOffset)
	{
		ImGui::TableSetColumnIndex(2);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yOffset);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 4.0f);
	}

	void InspectorPanel::DrawEmptyResetCell()
	{
		PreparePropertyResetCell(GetInspectorLabelYOffset());
		ImGui::Dummy(ImVec2(0.0f, 0.0f));
	}

	bool InspectorPanel::DrawResetButtonCell(const char* id, bool enabled)
	{
		PreparePropertyResetCell(GetInspectorControlYOffset());
		ImGui::PushID(id);
		if (!enabled)
		{
			ImGui::BeginDisabled();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		const bool clicked = ImGui::Button("R", ImVec2(inspectorResetColumn_Width - 8.0f, 0.0f));
		ImGui::PopStyleVar();

		if (!enabled)
		{
			ImGui::EndDisabled();
		}
		ImGui::PopID();
		return enabled && clicked;
	}

	void InspectorPanel::DrawInfoRow(const char* label, const std::string& value, bool& isHighlight)
	{
		BeginPropertyRow(isHighlight);
		DrawPropertyLabelCell(label);
		PreparePropertyValueCell(GetInspectorLabelYOffset());
		ImGui::TextUnformatted(value.c_str());
		DrawEmptyResetCell();
	}

	void InspectorPanel::DrawVec3Row(const char* label, glm::vec3& value, bool& isHighlight, float speed, const glm::vec3& defaultValue)
	{
		BeginPropertyRow(isHighlight);
		DrawPropertyLabelCell(label);
		PreparePropertyValueCell(GetInspectorControlYOffset());

		const float availWidth = ImGui::GetContentRegionAvail().x - inspectorValueRightInset;
		const float itemWidth = (availWidth - itemSpacingX * 2.0f) / 3.0f;
		const ImU32 axisColors[3] = {
			IM_COL32(214, 53, 53, 255),
			IM_COL32(76, 175, 80, 255),
			IM_COL32(66, 99, 235, 255)
		};
		const char* axisIds[3] = { "##X", "##Y", "##Z" };

		ImGui::PushID(label);
		for (int i = 0; i < 3; i++)
		{
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.09f, 0.09f, 0.09f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.11f, 0.11f, 0.11f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.09f, 0.09f, 0.09f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.30f, 0.30f, 0.30f, 1.0f));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::DragFloat(axisIds[i], (&value.x) + i, speed, 0.0f, 0.0f, "%.3f");

			const ImVec2 itemMin = ImGui::GetItemRectMin();
			const ImVec2 itemMax = ImGui::GetItemRectMax();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const float markerRight = itemMin.x - 2.0f;
			const float markerLeft = markerRight - 3.4f;
			const float markerTop = itemMin.y + 0.8f;
			const float markerBottom = itemMax.y - 0.2f;
			const float markerLeftTopInset = 1.7f;
			const float markerLeftBottomInset = 1.7f;
			const ImVec2 markerPoints[4] = {
				ImVec2(markerLeft, markerTop + markerLeftTopInset),
				ImVec2(markerRight, markerTop),
				ImVec2(markerRight, markerBottom),
				ImVec2(markerLeft, markerBottom - markerLeftBottomInset)
			};

			drawList->AddConvexPolyFilled(markerPoints, 4, axisColors[i]);
			drawList->AddPolyline(markerPoints, 4, IM_COL32(255, 255, 255, 36), ImDrawFlags_Closed, 1.2f);
			ImGui::PopStyleColor(4);

			if (i < 2)
			{
				ImGui::SameLine(0.0f, itemSpacingX);
			}
		}
		ImGui::PopID();

		const bool canReset = value != defaultValue;
		if (DrawResetButtonCell(label, canReset))
		{
			value = defaultValue;
		}
	}

	void InspectorPanel::DrawFloatRow(const char* label, float& value, bool& isHighlight, float speed, float minValue, float maxValue, float defaultValue)
	{
		BeginPropertyRow(isHighlight);
		DrawPropertyLabelCell(label);
		PreparePropertyValueCell(GetInspectorControlYOffset());

		ImGui::PushID(label);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.07f, 0.09f, 0.12f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.10f, 0.12f, 0.16f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.15f, 0.20f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 1.0f));
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - inspectorValueRightInset);
		ImGui::DragFloat("##FloatValue", &value, speed, minValue, maxValue, "%.3f");
		ImGui::PopStyleColor(4);
		ImGui::PopID();

		const bool canReset = value != defaultValue;
		if (DrawResetButtonCell(label, canReset))
		{
			value = defaultValue;
		}
	}

	void InspectorPanel::DrawColorRow(const char* label, glm::vec4& value, bool& isHighlight, const glm::vec4& defaultValue)
	{
		BeginPropertyRow(isHighlight);
		DrawPropertyLabelCell(label);
		PreparePropertyValueCell(GetInspectorControlYOffset());

		ImGui::PushID(label);
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 1.0f));
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - inspectorValueRightInset);
		ImGui::ColorEdit4("##ColorValue", &value.x,
			ImGuiColorEditFlags_DisplayRGB |
			ImGuiColorEditFlags_Float |
			ImGuiColorEditFlags_AlphaBar);
		ImGui::PopStyleColor();
		ImGui::PopID();

		const bool canReset = value != defaultValue;
		if (DrawResetButtonCell(label, canReset))
		{
			value = defaultValue;
		}
	}

	void InspectorPanel::DrawAssetSelectionRow(const char* label, AssetHandle& value, const std::vector<AssetMetadata>& assets, bool& isHighlight, AssetHandle defaultValue)
	{
		BeginPropertyRow(isHighlight);
		DrawPropertyLabelCell(label);
		PreparePropertyValueCell(GetInspectorControlYOffset());

		std::string selectedPath = "None";
		if (Asset::IsValidHandle(value))
		{
			if (const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(value))
			{
				selectedPath = metadata->relativePath.generic_string();
			}
		}

		ImGui::PushID(label);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.07f, 0.09f, 0.12f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.10f, 0.12f, 0.16f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.15f, 0.20f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 1.0f));
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - inspectorValueRightInset);
		if (ImGui::BeginCombo("##AssetSelector", selectedPath.c_str()))
		{
			const bool isNoneSelected = !Asset::IsValidHandle(value);
			if (ImGui::Selectable("None", isNoneSelected))
			{
				value = InvalidAssetHandle;
				selectedPath = "None";
			}

			if (isNoneSelected)
			{
				ImGui::SetItemDefaultFocus();
			}

			for (const auto& metadata : assets)
			{
				const std::string optionLabel = metadata.relativePath.generic_string();
				const bool isSelected = value == metadata.handle;
				if (ImGui::Selectable(optionLabel.c_str(), isSelected))
				{
					value = metadata.handle;
					selectedPath = optionLabel;
				}

				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopStyleColor(4);
		ImGui::PopID();

		const bool canReset = value != defaultValue;
		if (DrawResetButtonCell(label, canReset))
		{
			value = defaultValue;
		}
	}

	std::string InspectorPanel::GetAssetDisplayName(AssetHandle handle)
	{
		if (!Asset::IsValidHandle(handle))
		{
			return "None";
		}

		if (const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(handle))
		{
			const std::string filename = metadata->relativePath.filename().string();
			if (!filename.empty())
			{
				return filename;
			}

			return metadata->relativePath.generic_string();
		}

		return "None";
	}

	std::string InspectorPanel::GetAssetPathLabel(AssetHandle handle)
	{
		if (!Asset::IsValidHandle(handle))
		{
			return "None";
		}

		if (const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(handle))
		{
			return metadata->relativePath.generic_string();
		}

		return "None";
	}

	void InspectorPanel::OpenMaterialEditor(AssetHandle materialHandle)
	{
		if (m_OpenAssetCallback && Asset::IsValidHandle(materialHandle))
		{
			m_OpenAssetCallback(materialHandle);
		}
	}

	void InspectorPanel::DrawMaterialSlotRow(
		const char* label,
		size_t slotIndex,
		AssetHandle& slotMaterialHandle,
		AssetHandle defaultMaterialHandle,
		const std::vector<AssetMetadata>& materialAssets,
		bool& isHighlight)
	{
		BeginPropertyRow(isHighlight);
		DrawPropertyLabelCell(label);
		PreparePropertyValueCell(GetInspectorControlYOffset());

		ImGui::PushID(static_cast<int>(slotIndex));

		const float valueWidth = ImGui::GetContentRegionAvail().x - inspectorValueRightInset;
		const float tileSize = GetInspectorContentHeight() + 10.0f;
		const float comboSpacing = 10.0f;
		const float comboWidth = ImMax(80.0f, valueWidth - tileSize - comboSpacing);
		const ImVec2 tileSizeVec(tileSize, tileSize);

		ImGui::InvisibleButton("##MaterialThumbnail", tileSizeVec);
		const ImRect thumbRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(thumbRect.Min, thumbRect.Max, IM_COL32(72, 72, 72, 255));
		drawList->AddRect(thumbRect.Min, thumbRect.Max, IM_COL32(20, 20, 20, 255), 0.0f, 0, 1.0f);
		drawList->AddLine(
			ImVec2(thumbRect.Min.x + 6.0f, thumbRect.Max.y - 6.0f),
			ImVec2(thumbRect.Max.x - 6.0f, thumbRect.Min.y + 6.0f),
			IM_COL32(105, 105, 105, 255),
			1.0f);
		drawList->AddText(
			ImVec2(thumbRect.Min.x + 7.0f, thumbRect.Min.y + 7.0f),
			IM_COL32(180, 180, 180, 255),
			"MAT");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("TODO: Material thumbnail preview");
		}

		ImGui::SameLine(0.0f, comboSpacing);

		const std::string materialName = GetAssetDisplayName(slotMaterialHandle);
		const std::string materialPath = GetAssetPathLabel(slotMaterialHandle);

		ImGui::BeginGroup();
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.11f, 0.11f, 0.11f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
		ImGui::SetNextItemWidth(comboWidth);
		if (ImGui::BeginCombo("##MaterialSelector", materialName.c_str()))
		{
			const bool useDefaultMaterial = !Asset::IsValidHandle(slotMaterialHandle);
			if (ImGui::Selectable("Use Default Material", useDefaultMaterial))
			{
				slotMaterialHandle = InvalidAssetHandle;
			}

			if (useDefaultMaterial)
			{
				ImGui::SetItemDefaultFocus();
			}

			for (const auto& metadata : materialAssets)
			{
				const std::string optionLabel = metadata.relativePath.filename().string().empty()
					? metadata.relativePath.generic_string()
					: metadata.relativePath.filename().string();
				const bool isSelected = slotMaterialHandle == metadata.handle;
				if (ImGui::Selectable(optionLabel.c_str(), isSelected))
				{
					slotMaterialHandle = metadata.handle;
				}

				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopStyleColor(4);

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			OpenMaterialEditor(Asset::IsValidHandle(slotMaterialHandle) ? slotMaterialHandle : defaultMaterialHandle);
		}

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.58f, 0.58f, 0.58f, 1.0f));
		ImGui::TextUnformatted(materialPath.c_str());
		ImGui::PopStyleColor();
		ImGui::EndGroup();
		ImGui::PopID();

		const bool canReset = slotMaterialHandle != defaultMaterialHandle;
		if (DrawResetButtonCell(label, canReset))
		{
			slotMaterialHandle = defaultMaterialHandle;
		}
	}

	void InspectorPanel::DrawStaticMeshSlotRow(
		const char* label,
		AssetHandle& meshHandle,
		const std::vector<AssetMetadata>& meshAssets,
		bool& isHighlight,
		AssetHandle defaultValue)
	{
		BeginPropertyRow(isHighlight);
		DrawPropertyLabelCell(label);
		PreparePropertyValueCell(GetInspectorControlYOffset());

		ImGui::PushID(label);

		const float valueWidth = ImGui::GetContentRegionAvail().x - inspectorValueRightInset;
		const float tileSize = GetInspectorContentHeight() + 10.0f;
		const float comboSpacing = 10.0f;
		const float comboWidth = ImMax(80.0f, valueWidth - tileSize - comboSpacing);
		const ImVec2 tileSizeVec(tileSize, tileSize);

		ImGui::InvisibleButton("##MeshThumbnail", tileSizeVec);
		const ImRect thumbRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->AddRectFilled(thumbRect.Min, thumbRect.Max, IM_COL32(68, 68, 68, 255));
		drawList->AddRect(thumbRect.Min, thumbRect.Max, IM_COL32(20, 20, 20, 255), 0.0f, 0, 1.0f);
		drawList->AddTriangleFilled(
			ImVec2(thumbRect.Min.x + tileSize * 0.30f, thumbRect.Max.y - tileSize * 0.28f),
			ImVec2(thumbRect.Min.x + tileSize * 0.70f, thumbRect.Max.y - tileSize * 0.28f),
			ImVec2(thumbRect.Min.x + tileSize * 0.50f, thumbRect.Min.y + tileSize * 0.26f),
			IM_COL32(150, 150, 150, 255));
		drawList->AddText(
			ImVec2(thumbRect.Min.x + 6.0f, thumbRect.Min.y + 6.0f),
			IM_COL32(185, 185, 185, 255),
			"MSH");
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("TODO: Static mesh thumbnail preview");
		}

		ImGui::SameLine(0.0f, comboSpacing);

		const std::string meshName = GetAssetDisplayName(meshHandle);
		const std::string meshPath = GetAssetPathLabel(meshHandle);

		ImGui::BeginGroup();
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.11f, 0.11f, 0.11f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.12f, 0.12f, 0.12f, 1.0f));
		ImGui::SetNextItemWidth(comboWidth);
		if (ImGui::BeginCombo("##MeshSelector", meshName.c_str()))
		{
			const bool isNoneSelected = !Asset::IsValidHandle(meshHandle);
			if (ImGui::Selectable("None", isNoneSelected))
			{
				meshHandle = InvalidAssetHandle;
			}

			if (isNoneSelected)
			{
				ImGui::SetItemDefaultFocus();
			}

			for (const auto& metadata : meshAssets)
			{
				const std::string optionLabel = metadata.relativePath.filename().string().empty()
					? metadata.relativePath.generic_string()
					: metadata.relativePath.filename().string();
				const bool isSelected = meshHandle == metadata.handle;
				if (ImGui::Selectable(optionLabel.c_str(), isSelected))
				{
					meshHandle = metadata.handle;
				}

				if (isSelected)
				{
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndCombo();
		}
		ImGui::PopStyleColor(4);

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (m_OpenAssetCallback && Asset::IsValidHandle(meshHandle))
			{
				m_OpenAssetCallback(meshHandle);
			}
		}

		ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.58f, 0.58f, 0.58f, 1.0f));
		ImGui::TextUnformatted(meshPath.c_str());
		ImGui::PopStyleColor();
		ImGui::EndGroup();
		ImGui::PopID();

		const bool canReset = meshHandle != defaultValue;
		if (DrawResetButtonCell(label, canReset))
		{
			meshHandle = defaultValue;
		}
	}

	

	void InspectorPanel::OnImGuiRender()
	{
		DrawInspectorPanel();
	}

	void InspectorPanel::DrawInspectorPanel()
	{
		ImGuiWindowClass viewportWindowClass{};
		viewportWindowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
		ImGui::SetNextWindowClass(&viewportWindowClass);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("Inspector");

		EditorSelectionItemType itemType = m_SelectionContext->GetSelectionType();

		DrawItemByType(itemType);
	
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void InspectorPanel::DrawSelectedObject(Object& selectedObject)
	{
		DrawComponentOverview(selectedObject);

		DrawComponentSection<Transform>(
			selectedObject,
			"Transform",
			[&](Transform& transform)
			{
				DrawTransformProperties(transform);
			},
			false);

		DrawComponentSection<MeshRenderer>(
			selectedObject,
			"Static Mesh",
			[&](MeshRenderer& meshRenderer)
			{
				DrawStaticMeshProperties(meshRenderer);
			},
			true,
			false);

		DrawComponentSection<MeshRenderer>(
			selectedObject,
			"Materials",
			[&](MeshRenderer& meshRenderer)
			{
				DrawMeshRendererProperties(meshRenderer);
			},
			true,
			false);

		DrawComponentSection<LightComponent>(
			selectedObject,
			"LightComponent",
			[&](LightComponent& lightComponent)
			{
				DrawLightComponentProperties(lightComponent);
			});
	}

	void InspectorPanel::DrawComponentOverview(Object& selectedObject)
	{
		const float contentMinX = ImGui::GetWindowContentRegionMin().x;
		const float contentMaxX = ImGui::GetWindowContentRegionMax().x;
		const float fullWidth = contentMaxX - contentMinX;
		const float headerRowHeight = 28.0f;
		const float treeRowHeight = 24.0f;
		const float overviewPadding = 8.0f;
		int optionalComponentCount = 0;
		if (selectedObject.HasComponent<MeshRenderer>())
		{
			optionalComponentCount++;
		}
		if (selectedObject.HasComponent<LightComponent>())
		{
			optionalComponentCount++;
		}

		const float childHeight =
			overviewPadding * 2.0f +
			headerRowHeight +
			4.0f +
			treeRowHeight +
			(optionalComponentCount > 0 ? optionalComponentCount * treeRowHeight : treeRowHeight);

		ImGui::SetCursorPosX(contentMinX);
		ImGui::Dummy(ImVec2(0.0f, 2.0f));

		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.04f, 0.04f, 0.04f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, overviewPadding));
		ImGui::SetCursorPosX(contentMinX);
		if (ImGui::BeginChild("##InspectorComponentOverview", ImVec2(fullWidth, childHeight), ImGuiChildFlags_Borders))
		{
			auto drawComponentNode = [&](const char* nodeLabel, bool& removeFlag)
			{
				ImGui::PushID(nodeLabel);
				ImGui::SetNextItemOpen(false, ImGuiCond_Always);
				ImGui::TreeNodeEx(nodeLabel,
					ImGuiTreeNodeFlags_Leaf |
					ImGuiTreeNodeFlags_NoTreePushOnOpen |
					ImGuiTreeNodeFlags_SpanAvailWidth);

				if (ImGui::BeginPopupContextItem("##ComponentNodeContext"))
				{
					if (ImGui::MenuItem("Remove"))
					{
						removeFlag = true;
					}
					ImGui::EndPopup();
				}

				ImGui::PopID();
			};

			bool removeMeshRenderer = false;
			bool removeLight = false;
			const std::string objectName = selectedObject.GetName();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, 4.0f));
			ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 1.0f);
			ImGui::TextUnformatted(objectName.c_str());
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar();

			const float buttonWidth = 150.0f;
			const float cursorY = ImGui::GetCursorPosY();
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImMax(0.0f, fullWidth - buttonWidth - 24.0f - ImGui::CalcTextSize(objectName.c_str()).x));
			ImGui::SetCursorPosY(cursorY - 2.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10.0f, 4.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
			if (ImGui::Button("Add Component", ImVec2(buttonWidth, headerRowHeight)))
			{
				ImGui::OpenPopup("##InspectorAddComponentMenu");
			}
			ImGui::PopStyleColor(4);
			ImGui::PopStyleVar(3);

			DrawAddComponentMenu(selectedObject);
			ImGui::Dummy(ImVec2(0.0f, 6.0f));

			const ImGuiTreeNodeFlags rootNodeFlags =
				ImGuiTreeNodeFlags_DefaultOpen |
				ImGuiTreeNodeFlags_OpenOnArrow |
				ImGuiTreeNodeFlags_SpanAvailWidth;
			const bool rootOpen = ImGui::TreeNodeEx("##InspectorObjectRoot", rootNodeFlags, "%s", objectName.c_str());
			bool anyNodeDrawn = false;

			if (rootOpen)
			{
				if (selectedObject.HasComponent<MeshRenderer>())
				{
					drawComponentNode("MeshRenderer", removeMeshRenderer);
					anyNodeDrawn = true;
				}

				if (selectedObject.HasComponent<LightComponent>())
				{
					drawComponentNode("Light", removeLight);
					anyNodeDrawn = true;
				}

				if (!anyNodeDrawn)
				{
					ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.55f, 0.55f, 1.0f));
					ImGui::TreeNodeEx("No optional components",
						ImGuiTreeNodeFlags_Leaf |
						ImGuiTreeNodeFlags_NoTreePushOnOpen |
						ImGuiTreeNodeFlags_SpanAvailWidth);
					ImGui::PopStyleColor();
				}

				ImGui::TreePop();
			}

			if (removeMeshRenderer)
			{
				selectedObject.RemoveComponent<MeshRenderer>();
			}
			if (removeLight)
			{
				selectedObject.RemoveComponent<LightComponent>();
			}
		}
		ImGui::EndChild();
		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(2);
		ImGui::Dummy(ImVec2(0.0f, 6.0f));
	}

	void InspectorPanel::DrawAddComponentMenu(Object& selectedObject)
	{
		(void)selectedObject;

		if (!ImGui::BeginPopup("##InspectorAddComponentMenu"))
		{
			return;
		}

		ImGui::TextUnformatted("Add Component");
		ImGui::Separator();

		if (ImGui::BeginMenu("Rendering"))
		{
			if (ImGui::MenuItem("Mesh Renderer"))
			{
				// TODO: Add MeshRenderer component to the selected object.
			}
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Lighting"))
		{
			if (ImGui::MenuItem("Light"))
			{
				// TODO: Add LightComponent to the selected object.
			}
			ImGui::EndMenu();
		}

		ImGui::EndPopup();
	}

	void InspectorPanel::DrawSelectedAsset(AssetHandle handle)
	{
	}

	void InspectorPanel::DrawTransformProperties(Transform& transform)
	{
		DrawComponentPropertyTable("##TransformComponentTable",
			[&](bool& isHighlight)
		{
			DrawVec3Row("Position", transform.GetPosition(), isHighlight, 0.05f, glm::vec3(0.0f));
			DrawVec3Row("Rotation", transform.GetRotation(), isHighlight, 0.05f, glm::vec3(0.0f));
			DrawVec3Row("Scale", transform.GetScale(), isHighlight, 0.05f, glm::vec3(1.0f));
		});
	}

	void InspectorPanel::DrawStaticMeshProperties(MeshRenderer& meshRenderer)
	{
		DrawComponentPropertyTable("##StaticMeshComponentTable",
			[&](bool& isHighlight)
		{
			auto& assetManager = AssetManager::GetInstance();
			const auto meshAssets = assetManager.GetAssetsByType(AssetType::Mesh);
			DrawStaticMeshSlotRow("Static Mesh",
				meshRenderer.MeshAssetHandle,
				meshAssets,
				isHighlight,
				InvalidAssetHandle);
		});
	}

	void InspectorPanel::DrawMeshRendererProperties(MeshRenderer& meshRenderer)
	{
		DrawComponentPropertyTable("##MaterialsComponentTable",
			[&](bool& isHighlight)
		{
			auto& assetManager = AssetManager::GetInstance();
			auto& materialHandles = meshRenderer.MaterialAssetHandles;
			const auto materialAssets = assetManager.GetAssetsByType(AssetType::Material);
			if (materialHandles.empty())
			{
				DrawMaterialSlotRow("Element 0",
					0,
					meshRenderer.DefaultMaterialAssetHandle,
					meshRenderer.DefaultMaterialAssetHandle,
					materialAssets,
					isHighlight);
				return;
			}

			for (size_t i = 0; i < materialHandles.size(); ++i)
			{
				DrawMaterialSlotRow(
					("Element " + std::to_string(i)).c_str(),
					i,
					materialHandles[i],
					meshRenderer.DefaultMaterialAssetHandle,
					materialAssets,
					isHighlight);
			}
		});
	}

	void InspectorPanel::DrawLightComponentProperties(LightComponent& lightComponent)
	{
		DrawComponentPropertyTable("##LightComponentTable",
			[&](bool& isHighlight)
		{
			glm::vec4 lightColor = lightComponent.color;
			DrawColorRow("Color", lightColor, isHighlight, glm::vec4(1.0f));
			if (lightColor.x != lightComponent.color.x ||
				lightColor.y != lightComponent.color.y ||
				lightColor.z != lightComponent.color.z ||
				lightColor.w != lightComponent.color.w)
			{
				lightComponent.color = lightColor;
			}

			float intensity = lightComponent.intensity;
			DrawFloatRow("Intensity", intensity, isHighlight, 0.05f, 0.0f, 10.0f, 1.0f);
			if (intensity != lightComponent.intensity)
			{
				lightComponent.intensity = intensity;
			}
		});
	}

}
