#include "renderer_pch.h"
#include "InspectorPanel.h"

#include "imgui.h"
#include <imgui_internal.h>

namespace Kita {

	namespace
	{
		constexpr float inspectorHeaderHeight = 28.0f;
		constexpr float inspectorItemSpacingX = 12.0f;
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
		DrawItemByType(m_SelectionContext ? m_SelectionContext->GetSelectionType() : EditorSelectionItemType::None);
		ImGui::End();

		ImGui::PopStyleVar();
	}

	void InspectorPanel::DrawItemByType(EditorSelectionItemType type)
	{
		switch (type)
		{
		case EditorSelectionItemType::SceneObject:
			DrawSelectedObject(m_SelectionContext->GetSelectionItemHandle().m_SelectionObject);
			return;
		case EditorSelectionItemType::Asset:
			DrawSelectedAsset(m_SelectionContext->GetSelectionItemHandle().m_SelectedAssetHandle);
			return;
		default:
			return;
		}
	}

	void InspectorPanel::DrawSelectedObject(Object& selectedObject)
	{
		DrawObjectHeader(selectedObject);

		if (DrawPropertyGroupHeader("Transform"))
		{
			DrawTransformProperties(selectedObject.GetComponent<Transform>());
			ImGui::TreePop();
		}

		if (selectedObject.HasComponent<MeshRenderer>())
		{
			auto& meshRenderer = selectedObject.GetComponent<MeshRenderer>();
			if (DrawPropertyGroupHeader("Static Mesh"))
			{
				DrawStaticMeshProperties(meshRenderer);
				ImGui::TreePop();
			}

			if (DrawPropertyGroupHeader("Material"))
			{
				DrawMaterialProperties(meshRenderer);
				ImGui::TreePop();
			}
		}

		if (selectedObject.HasComponent<LightComponent>())
		{
			if (DrawPropertyGroupHeader("Light"))
			{
				DrawLightProperties(selectedObject.GetComponent<LightComponent>());
				ImGui::TreePop();
			}
		}
	}

	void InspectorPanel::DrawSelectedAsset(AssetHandle handle)
	{
		if (!Asset::IsValidHandle(handle))
		{
			return;
		}

		if (UIAttributeUtil::BeginPropertyTable("##InspectorSelectedAssetTable", m_TableStyle))
		{
			DrawInfoRow("Asset", GetAssetDisplayName(handle));
			DrawInfoRow("Path", GetAssetPathLabel(handle));
			UIAttributeUtil::EndPropertyTable();
		}
	}

	void InspectorPanel::DrawObjectHeader(Object& selectedObject)
	{
		const float contentMinX = ImGui::GetWindowContentRegionMin().x;
		const float contentMaxX = ImGui::GetWindowContentRegionMax().x;
		const float fullWidth = contentMaxX - contentMinX;
		const std::string objectName = selectedObject.GetName();

		ImGui::SetCursorPosX(contentMinX);
		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.04f, 0.04f, 0.04f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_ChildBorderSize, 1.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8.0f, 8.0f));
		if (ImGui::BeginChild("##InspectorObjectHeader", ImVec2(fullWidth, 44.0f), ImGuiChildFlags_Borders))
		{
			ImGui::AlignTextToFramePadding();
			ImGui::TextUnformatted(objectName.c_str());

			const float buttonWidth = 120.0f;
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetWindowContentRegionMax().x - buttonWidth);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.18f, 0.18f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
			if (ImGui::Button("Add Component", ImVec2(buttonWidth, inspectorHeaderHeight)))
			{
				ImGui::OpenPopup("##InspectorAddComponentMenu");
			}
			ImGui::PopStyleColor(4);
			ImGui::PopStyleVar(2);

			DrawAddComponentMenu(selectedObject);
		}
		ImGui::EndChild();
		ImGui::PopStyleVar(3);
		ImGui::PopStyleColor(2);
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

	bool InspectorPanel::DrawPropertyGroupHeader(const char* label)
	{
		const ImGuiTreeNodeFlags headerFlags =
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_Framed |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_FramePadding;
		const float textPaddingY = (30.0f - ImGui::GetTextLineHeight()) * 0.5f;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.0f, textPaddingY));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.23f, 0.23f, 0.23f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.23f, 0.23f, 0.23f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.23f, 0.23f, 0.23f, 1.0f));
		const bool open = ImGui::TreeNodeEx(label, headerFlags, "%s", label);
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(2);
		return open;
	}

	void InspectorPanel::DrawTransformProperties(Transform& transform)
	{
		if (UIAttributeUtil::BeginPropertyTable("##InspectorTransformTable", m_TableStyle))
		{
			DrawVec3Row("Position", transform.GetPosition(), 0.05f, glm::vec3(0.0f));
			DrawVec3Row("Rotation", transform.GetRotation(), 0.05f, glm::vec3(0.0f));
			DrawVec3Row("Scale", transform.GetScale(), 0.05f, glm::vec3(1.0f));
			UIAttributeUtil::EndPropertyTable();
		}
	}

	void InspectorPanel::DrawStaticMeshProperties(MeshRenderer& meshRenderer)
	{
		if (UIAttributeUtil::BeginPropertyTable("##InspectorStaticMeshTable", m_TableStyle))
		{
			const auto meshAssets = AssetManager::GetInstance().GetAssetsByType(AssetType::Mesh);
			DrawStaticMeshSlotRow("Static Mesh", meshRenderer.MeshAssetHandle, meshAssets, InvalidAssetHandle);
			UIAttributeUtil::EndPropertyTable();
		}
	}

	void InspectorPanel::DrawMaterialProperties(MeshRenderer& meshRenderer)
	{
		if (UIAttributeUtil::BeginPropertyTable("##InspectorMaterialTable", m_TableStyle))
		{
			const auto materialAssets = AssetManager::GetInstance().GetAssetsByType(AssetType::Material);
			auto& materialHandles = meshRenderer.MaterialAssetHandles;

			if (materialHandles.empty())
			{
				DrawMaterialSlotRow("Element 0", 0, meshRenderer.DefaultMaterialAssetHandle, meshRenderer.DefaultMaterialAssetHandle, materialAssets);
			}
			else
			{
				for (size_t i = 0; i < materialHandles.size(); ++i)
				{
					DrawMaterialSlotRow(("Element " + std::to_string(i)).c_str(), i, materialHandles[i], meshRenderer.DefaultMaterialAssetHandle, materialAssets);
				}
			}

			UIAttributeUtil::EndPropertyTable();
		}
	}

	void InspectorPanel::DrawLightProperties(LightComponent& lightComponent)
	{
		if (UIAttributeUtil::BeginPropertyTable("##InspectorLightTable", m_TableStyle))
		{
			glm::vec4 lightColor = lightComponent.color;
			DrawColorRow("Color", lightColor, glm::vec4(1.0f));
			if (lightColor != lightComponent.color)
			{
				lightComponent.color = lightColor;
			}

			float intensity = lightComponent.intensity;
			DrawFloatRow("Intensity", intensity, 0.05f, 0.0f, 10.0f, 1.0f);
			if (intensity != lightComponent.intensity)
			{
				lightComponent.intensity = intensity;
			}

			UIAttributeUtil::EndPropertyTable();
		}
	}

	void InspectorPanel::DrawInfoRow(const char* label, const std::string& value)
	{
		UIAttributeUtil::BeginPropertyRow(m_TableStyle);
		UIAttributeUtil::DrawPropertyLabelCell(label, m_TableStyle);
		UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetLabelYOffset(m_TableStyle));
		ImGui::TextUnformatted(value.c_str());
		UIAttributeUtil::DrawEmptyResetCell(m_TableStyle);
	}

	void InspectorPanel::DrawVec3Row(const char* label, glm::vec3& value, float speed, const glm::vec3& defaultValue)
	{
		UIAttributeUtil::BeginPropertyRow(m_TableStyle);
		UIAttributeUtil::DrawPropertyLabelCell(label, m_TableStyle);
		UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetControlYOffset(m_TableStyle));

		const float availWidth = ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset;
		const float itemWidth = (availWidth - inspectorItemSpacingX * 2.0f) / 3.0f;
		const ImU32 axisColors[3] = {
			IM_COL32(214, 53, 53, 255),
			IM_COL32(76, 175, 80, 255),
			IM_COL32(66, 99, 235, 255)
		};
		const char* axisIds[3] = { "##X", "##Y", "##Z" };

		ImGui::PushID(label);
		for (int i = 0; i < 3; ++i)
		{
			UIAttributeUtil::PushInputStyle(m_TableStyle);
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::DragFloat(axisIds[i], (&value.x) + i, speed, 0.0f, 0.0f, "%.3f");

			const ImVec2 itemMin = ImGui::GetItemRectMin();
			const ImVec2 itemMax = ImGui::GetItemRectMax();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const ImVec2 markerPoints[4] = {
				ImVec2(itemMin.x - 5.4f, itemMin.y + 2.5f),
				ImVec2(itemMin.x - 2.0f, itemMin.y + 0.8f),
				ImVec2(itemMin.x - 2.0f, itemMax.y - 0.2f),
				ImVec2(itemMin.x - 5.4f, itemMax.y - 1.9f)
			};

			drawList->AddConvexPolyFilled(markerPoints, 4, axisColors[i]);
			drawList->AddPolyline(markerPoints, 4, IM_COL32(255, 255, 255, 36), ImDrawFlags_Closed, 1.2f);
			UIAttributeUtil::PopInputStyle();

			if (i < 2)
			{
				ImGui::SameLine(0.0f, inspectorItemSpacingX);
			}
		}
		ImGui::PopID();

		if (UIAttributeUtil::DrawResetButtonCell(label, m_TableStyle, value != defaultValue))
		{
			value = defaultValue;
		}
	}

	void InspectorPanel::DrawFloatRow(const char* label, float& value, float speed, float minValue, float maxValue, float defaultValue)
	{
		UIAttributeUtil::BeginPropertyRow(m_TableStyle);
		UIAttributeUtil::DrawPropertyLabelCell(label, m_TableStyle);
		UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetControlYOffset(m_TableStyle));

		ImGui::PushID(label);
		UIAttributeUtil::PushInputStyle(m_TableStyle);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset);
		ImGui::DragFloat("##FloatValue", &value, speed, minValue, maxValue, "%.3f");
		UIAttributeUtil::PopInputStyle();
		ImGui::PopID();

		if (UIAttributeUtil::DrawResetButtonCell(label, m_TableStyle, value != defaultValue))
		{
			value = defaultValue;
		}
	}

	void InspectorPanel::DrawColorRow(const char* label, glm::vec4& value, const glm::vec4& defaultValue)
	{
		UIAttributeUtil::BeginPropertyRow(m_TableStyle);
		UIAttributeUtil::DrawPropertyLabelCell(label, m_TableStyle);
		UIAttributeUtil::PreparePropertyValueCell(m_TableStyle, UIAttributeUtil::GetControlYOffset(m_TableStyle));

		ImGui::PushID(label);
		ImGui::PushStyleColor(ImGuiCol_Border, m_TableStyle.InputBorderColor);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset);
		ImGui::ColorEdit4("##ColorValue", &value.x,
			ImGuiColorEditFlags_DisplayRGB |
			ImGuiColorEditFlags_Float |
			ImGuiColorEditFlags_AlphaBar);
		ImGui::PopStyleColor();
		ImGui::PopID();

		if (UIAttributeUtil::DrawResetButtonCell(label, m_TableStyle, value != defaultValue))
		{
			value = defaultValue;
		}
	}

	void InspectorPanel::DrawStaticMeshSlotRow(const char* label, AssetHandle& meshHandle, const std::vector<AssetMetadata>& meshAssets, AssetHandle defaultValue)
	{
		UIAttributeUtil::BeginPropertyRow(m_TableStyle, m_TableStyle.PreviewRowHeight);
		UIAttributeUtil::DrawPropertyLabelCell(label, m_TableStyle);
		UIAttributeUtil::PreparePropertyValueCell(
			m_TableStyle,
			UIAttributeUtil::GetControlYOffset(m_TableStyle, m_TableStyle.PreviewRowHeight, m_TableStyle.PreviewTileSize));

		ImGui::PushID(label);
		const float valueWidth = ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset;
		const float tileSize = m_TableStyle.PreviewTileSize;
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

		ImGui::SameLine(0.0f, comboSpacing);
		UIAttributeUtil::PushInputStyle(m_TableStyle);
		if (UIAttributeUtil::DrawAssetCombo("##MeshSelector", meshHandle, meshAssets, comboWidth, UIAttributeUtil::AssetLabelMode::FileName))
		{
		}
		UIAttributeUtil::PopInputStyle();

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (m_OpenAssetCallback && Asset::IsValidHandle(meshHandle))
			{
				m_OpenAssetCallback(meshHandle);
			}
		}

		ImGui::PopID();

		if (UIAttributeUtil::DrawResetButtonCell(
			label,
			m_TableStyle,
			meshHandle != defaultValue,
			UIAttributeUtil::GetControlYOffset(m_TableStyle, m_TableStyle.PreviewRowHeight)))
		{
			meshHandle = defaultValue;
		}
	}

	void InspectorPanel::DrawMaterialSlotRow(
		const char* label,
		size_t slotIndex,
		AssetHandle& slotMaterialHandle,
		AssetHandle defaultMaterialHandle,
		const std::vector<AssetMetadata>& materialAssets)
	{
		UIAttributeUtil::BeginPropertyRow(m_TableStyle, m_TableStyle.PreviewRowHeight);
		UIAttributeUtil::DrawPropertyLabelCell(label, m_TableStyle);
		UIAttributeUtil::PreparePropertyValueCell(
			m_TableStyle,
			UIAttributeUtil::GetControlYOffset(m_TableStyle, m_TableStyle.PreviewRowHeight, m_TableStyle.PreviewTileSize));

		ImGui::PushID(static_cast<int>(slotIndex));
		const float valueWidth = ImGui::GetContentRegionAvail().x - m_TableStyle.ValueRightInset;
		const float tileSize = m_TableStyle.PreviewTileSize;
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

		ImGui::SameLine(0.0f, comboSpacing);
		UIAttributeUtil::PushInputStyle(m_TableStyle);
		if (UIAttributeUtil::DrawAssetCombo(
			"##MaterialSelector",
			slotMaterialHandle,
			materialAssets,
			comboWidth,
			UIAttributeUtil::AssetLabelMode::FileName,
			"Use Default Material"))
		{
		}
		UIAttributeUtil::PopInputStyle();

		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			OpenMaterialEditor(Asset::IsValidHandle(slotMaterialHandle) ? slotMaterialHandle : defaultMaterialHandle);
		}

		ImGui::PopID();

		if (UIAttributeUtil::DrawResetButtonCell(
			label,
			m_TableStyle,
			slotMaterialHandle != defaultMaterialHandle,
			UIAttributeUtil::GetControlYOffset(m_TableStyle, m_TableStyle.PreviewRowHeight)))
		{
			slotMaterialHandle = defaultMaterialHandle;
		}
	}

	void InspectorPanel::OpenMaterialEditor(AssetHandle materialHandle)
	{
		if (m_OpenAssetCallback && Asset::IsValidHandle(materialHandle))
		{
			m_OpenAssetCallback(materialHandle);
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
			return filename.empty() ? metadata->relativePath.generic_string() : filename;
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

}
