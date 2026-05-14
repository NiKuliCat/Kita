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
			"MeshRenderer",
			[&](MeshRenderer& meshRenderer)
			{
				DrawMeshRendererProperties(meshRenderer);
			});

		DrawComponentSection<LightComponent>(
			selectedObject,
			"LightComponent",
			[&](LightComponent& lightComponent)
			{
				DrawLightComponentProperties(lightComponent);
			});
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

	void InspectorPanel::DrawMeshRendererProperties(MeshRenderer& meshRenderer)
	{
		DrawComponentPropertyTable("##MeshRendererComponentTable",
			[&](bool& isHighlight)
		{
			auto& assetManager = AssetManager::GetInstance();

			std::string meshSource = "None";
			size_t subMeshCount = 0;
			if (Asset::IsValidHandle(meshRenderer.MeshAssetHandle))
			{
				if (const AssetMetadata* metadata = assetManager.GetMetadata(meshRenderer.MeshAssetHandle))
				{
					meshSource = metadata->relativePath.generic_string();
				}

				if (Ref<MeshAsset> meshAsset = assetManager.GetMeshAsset(meshRenderer.MeshAssetHandle))
				{
					subMeshCount = meshAsset->MeshRawData.size();
				}
			}

			DrawInfoRow("Mesh Source", meshSource, isHighlight);
			DrawInfoRow("SubMesh Count", std::to_string(subMeshCount), isHighlight);

			auto& materialHandles = meshRenderer.MaterialAssetHandles;
			const auto shaderAssets = assetManager.GetAssetsByType(AssetType::Shader);
			const auto textureAssets = assetManager.GetAssetsByType(AssetType::Texture);
			for (size_t i = 0; i < materialHandles.size(); ++i)
			{
				const AssetHandle materialHandle = materialHandles[i];
				Ref<MaterialAsset> materialAsset = assetManager.GetMaterialAsset(materialHandle);
				if (!materialAsset)
				{
					DrawInfoRow(("Material " + std::to_string(i)).c_str(), "None", isHighlight);
					continue;
				}

				DrawInfoRow(("Material " + std::to_string(i)).c_str(), "Slot", isHighlight);
				DrawAssetSelectionRow(("Shader " + std::to_string(i)).c_str(),
					materialAsset->ShaderHandle, shaderAssets, isHighlight, InvalidAssetHandle);
				DrawAssetSelectionRow(("Albedo " + std::to_string(i)).c_str(),
					materialAsset->AlbedoTextureHandle, textureAssets, isHighlight, InvalidAssetHandle);

				glm::vec4 baseColor = materialAsset->BaseColor;
				DrawColorRow(("Base Color " + std::to_string(i)).c_str(), baseColor, isHighlight, glm::vec4(1.0f));
				if (baseColor != materialAsset->BaseColor)
				{
					materialAsset->BaseColor = baseColor;
				}
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
