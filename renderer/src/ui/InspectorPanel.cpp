#include "renderer_pch.h"
#include "InspectorPanel.h"
#include "imgui.h"
#include "render/ShaderLibrary.h"
#include <imgui_internal.h>

namespace Kita {
	const float inspectorLabelColumn_LeftWidth = 130.0f;

	const float rowHeight = 32.0f;
	const float textPaddingX = 10.0f;
	const float itemSpacingX = 12.0f;
	const ImVec2 inspectorControlFramePadding = ImVec2(6.0f, 2.0f);
	const float inspectorControlFrameRounding = 3.0f;
	const float inspectorControlFrameBorderSize = 1.0f;

	const ImU32 separatorColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.22f, 0.22f, 0.22f, 1.0f));
	const ImU32 tableBgColor_Dark = ImGui::ColorConvertFloat4ToU32(ImVec4(0.13f, 0.13f, 0.13f, 1.0f));
	const ImU32 tableBgColor_Light = ImGui::ColorConvertFloat4ToU32(ImVec4(0.19f, 0.19f, 0.19f, 1.0f));

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

	const char* InspectorPanel::ObjectTypeToString(Type type)
	{
		switch (type)
		{
		case Type::StaticMesh: return "StaticMesh";
		case Type::Curve:      return "Curve";
		default:               return "Unknown";
		}
	}

	bool InspectorPanel::BeginPropertyTable(const char* id)
	{
		const ImGuiTableFlags tableFlags =
			ImGuiTableFlags_SizingStretchProp |
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_BordersOuter |
			ImGuiTableFlags_NoSavedSettings;

		ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, separatorColor);
		ImGui::PushStyleColor(ImGuiCol_TableBorderLight, separatorColor);
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.30f, 0.30f, 0.30f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, inspectorControlFramePadding);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, inspectorControlFrameRounding);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, inspectorControlFrameBorderSize);

		const float tableMinX = ImGui::GetWindowContentRegionMin().x;
		const float tableMaxX = ImGui::GetWindowContentRegionMax().x;
		ImGui::SetCursorPosX(tableMinX);

		const bool open = ImGui::BeginTable(id, 2, tableFlags, ImVec2(tableMaxX - tableMinX, 0.0f));
		if (!open)
		{
			ImGui::PopStyleColor(3);
			ImGui::PopStyleVar(3);
			return false;
		}

		ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, inspectorLabelColumn_LeftWidth);
		ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);
		return true;
	}

	void InspectorPanel::EndPropertyTable()
	{
		ImGui::EndTable();
		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar(3);
	}

	void InspectorPanel::BeginPropertyRow(bool& isHighlight)
	{
		ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
		const ImU32 bgColor = isHighlight ? tableBgColor_Light : tableBgColor_Dark;
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bgColor);
		isHighlight = !isHighlight;
	}

	void InspectorPanel::DrawPropertyLabelCell(const char* label)
	{
		ImGui::TableSetColumnIndex(0);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + GetInspectorLabelYOffset());
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPaddingX);
		ImGui::TextUnformatted(label);
	}

	void InspectorPanel::PreparePropertyValueCell(float yOffset)
	{
		ImGui::TableSetColumnIndex(1);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yOffset);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPaddingX);
	}

	void InspectorPanel::DrawInfoRow(const char* label, const std::string& value, bool& isHighlight)
	{
		BeginPropertyRow(isHighlight);
		DrawPropertyLabelCell(label);
		PreparePropertyValueCell(GetInspectorLabelYOffset());
		ImGui::TextUnformatted(value.c_str());
	}

	void InspectorPanel::DrawVec3Row(const char* label, glm::vec3& value, bool& isHighlight, float speed)
	{
		BeginPropertyRow(isHighlight);
		DrawPropertyLabelCell(label);
		PreparePropertyValueCell(GetInspectorControlYOffset());

		const float availWidth = ImGui::GetContentRegionAvail().x - textPaddingX;
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
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.15f, 0.20f, 1.0f));
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
	}

	void InspectorPanel::DrawFloatRow(const char* label, float& value, bool& isHighlight, float speed, float minValue)
	{
		BeginPropertyRow(isHighlight);
		DrawPropertyLabelCell(label);
		PreparePropertyValueCell(GetInspectorControlYOffset());

		ImGui::PushID(label);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.07f, 0.09f, 0.12f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.10f, 0.12f, 0.16f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.15f, 0.20f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 1.0f));
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - textPaddingX);
		ImGui::DragFloat("##FloatValue", &value, speed, minValue, 0.0f, "%.3f");
		ImGui::PopStyleColor(4);
		ImGui::PopID();
	}

	void InspectorPanel::DrawColorRow(const char* label, glm::vec4& value, bool& isHighlight)
	{
		BeginPropertyRow(isHighlight);
		DrawPropertyLabelCell(label);
		PreparePropertyValueCell(GetInspectorControlYOffset());

		ImGui::PushID(label);
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 1.0f));
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - textPaddingX);
		ImGui::ColorEdit4("##ColorValue", &value.x,
			ImGuiColorEditFlags_DisplayRGB |
			ImGuiColorEditFlags_Float |
			ImGuiColorEditFlags_AlphaBar);
		ImGui::PopStyleColor();
		ImGui::PopID();
	}

	void InspectorPanel::DrawCurveTypeRow(const char* label, CurveType& curveType, bool& isHighlight)
	{
		BeginPropertyRow(isHighlight);
		DrawPropertyLabelCell(label);
		PreparePropertyValueCell(GetInspectorControlYOffset());

		const char* items[] = { "Polyline", "BezierCubic" };
		int currentIndex = static_cast<int>(curveType);

		ImGui::PushID(label);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.07f, 0.09f, 0.12f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.10f, 0.12f, 0.16f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.15f, 0.20f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 1.0f));
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - textPaddingX);
		if (ImGui::Combo("##CurveType", &currentIndex, items, IM_ARRAYSIZE(items)))
		{
			curveType = static_cast<CurveType>(currentIndex);
		}
		ImGui::PopStyleColor(4);
		ImGui::PopID();
	}

	void InspectorPanel::DrawPointCountRow(LineRenderer& lineRenderer, PointData& selectedPoint, bool& isHighlight)
	{
		BeginPropertyRow(isHighlight);
		DrawPropertyLabelCell("Point Count");
		PreparePropertyValueCell(GetInspectorControlYOffset());

		const uint32_t anchorCount = lineRenderer.GetControlPointCount() == 0
			? 0
			: static_cast<uint32_t>((lineRenderer.GetControlPointCount() + 2) / 3);
		const std::string pointCountText = std::to_string(anchorCount);
		const float totalWidth = ImGui::GetContentRegionAvail().x - textPaddingX;
		const float buttonWidth = 30.0f;
		const float spacing = 8.0f;
		const float textWidth = totalWidth - buttonWidth * 2.0f - spacing * 2.0f;
		const bool canEditSegments = lineRenderer.GetCurveType() == CurveType::BezierCubic;

		ImGui::PushID("PointCountRow");
		ImGui::SetNextItemWidth(textWidth);
		ImGui::InputText("##PointCount", const_cast<char*>(pointCountText.c_str()), static_cast<size_t>(pointCountText.size() + 1), ImGuiInputTextFlags_ReadOnly);
		ImGui::SameLine(0.0f, spacing);
		if (!canEditSegments)
		{
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("+", ImVec2(buttonWidth, 0.0f)))
		{
			lineRenderer.AppendBezierSegment();
		}
		ImGui::SameLine(0.0f, spacing);
		if (ImGui::Button("-", ImVec2(buttonWidth, 0.0f)))
		{
			lineRenderer.RemoveLastBezierSegment();
			if (selectedPoint.id >= static_cast<int>(lineRenderer.GetControlPointCount()))
			{
				ClearSelectedPoint();
			}
		}
		if (!canEditSegments)
		{
			ImGui::EndDisabled();
		}
		ImGui::PopID();
	}

	void InspectorPanel::DrawAnchorRow(const char* label, glm::vec3& value, BezierHandleMode& handleMode, bool& isHighlight)
	{
		BeginPropertyRow(isHighlight);
		DrawPropertyLabelCell(label);
		PreparePropertyValueCell(GetInspectorControlYOffset());

		const float availableWidth = ImGui::GetContentRegionAvail().x - textPaddingX;
		const float comboWidth = 110.0f;
		const float spacing = 10.0f;
		const float vec3Width = availableWidth - comboWidth - spacing;
		const float singleAxisSpacing = 8.0f;
		const float itemWidth = (vec3Width - singleAxisSpacing * 2.0f) / 3.0f;
		const ImU32 axisColors[3] = {
			IM_COL32(214, 53, 53, 255),
			IM_COL32(76, 175, 80, 255),
			IM_COL32(66, 99, 235, 255)
		};
		const char* axisIds[3] = { "##X", "##Y", "##Z" };
		const char* items[] = { "Free", "Aligned", "Mirrored" };
		int currentIndex = static_cast<int>(handleMode);

		ImGui::PushID(label);
		for (int i = 0; i < 3; i++)
		{
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.14f, 0.14f, 0.14f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.17f, 0.17f, 0.17f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.15f, 0.20f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.30f, 0.30f, 0.30f, 1.0f));
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::DragFloat(axisIds[i], (&value.x) + i, 0.05f, 0.0f, 0.0f, "%.3f");

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

			ImGui::SameLine(0.0f, i < 2 ? singleAxisSpacing : spacing);
		}

		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.07f, 0.09f, 0.12f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.10f, 0.12f, 0.16f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.15f, 0.20f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 1.0f));
		ImGui::SetNextItemWidth(comboWidth);
		if (ImGui::Combo("##HandleMode", &currentIndex, items, IM_ARRAYSIZE(items)))
		{
			handleMode = static_cast<BezierHandleMode>(currentIndex);
		}
		ImGui::PopStyleColor(4);
		ImGui::PopID();
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

		ImGui::Begin("Inspector");

		auto& selectedObject = GetSelectedObject();
		auto& selectedPoint = GetSelectedPoint();
		if (selectedObject)
		{
			DrawSelectedObject(selectedObject, selectedPoint);
		}

		ImGui::End();
	}

	void InspectorPanel::DrawSelectedObject(Object& selectedObject, PointData& selectedPoint)
	{
		DrawObjectInfoSection(selectedObject);

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

		DrawComponentSection<LineRenderer>(
			selectedObject,
			"LineRenderer",
			[&](LineRenderer& lineRenderer)
			{
				DrawLineRendererProperties(lineRenderer, selectedPoint);
				DrawLineRendererControlPoints(lineRenderer, selectedPoint);
			});
	}

	void InspectorPanel::DrawObjectInfoSection(Object& selectedObject)
	{
		if (!BeginPropertyTable("##InspectorObjectInfoTable"))
		{
			return;
		}

		bool isHighlight = false;

		if (selectedObject.HasComponent<Name>())
		{
			auto& name = selectedObject.GetComponent<Name>().Get();
			DrawInfoRow("Name", name, isHighlight);
		}

		if (selectedObject.HasComponent<ObjectType>())
		{
			const Type& type = selectedObject.GetComponent<ObjectType>().Get();
			DrawInfoRow("Type", ObjectTypeToString(type), isHighlight);
		}

		if (selectedObject.HasComponent<Transform>())
		{
			auto& transform = selectedObject.GetComponent<Transform>();
			DrawVec3Row("Position", transform.GetPosition(), isHighlight);
			DrawVec3Row("Rotation", transform.GetRotation(), isHighlight);
			DrawVec3Row("Scale", transform.GetScale(), isHighlight);
		}

		EndPropertyTable();
	}

	void InspectorPanel::DrawMeshRendererProperties(MeshRenderer& meshRenderer)
	{
		const float treeIndent = ImGui::GetTreeNodeToLabelSpacing();
		ImGui::Unindent(treeIndent);

		if (BeginPropertyTable("##MeshRendererComponentTable"))
		{
			bool isHighlight = false;

			DrawInfoRow("Mesh Source", meshRenderer.GetMeshFilePath(), isHighlight);
			DrawInfoRow("SubMesh Count", std::to_string(meshRenderer.GetSubMeshCount()), isHighlight);

			auto shaderNames = ShaderLibrary::GetInstance().GetShaderNames();
			auto& materials = meshRenderer.GetMaterials();
			for (size_t i = 0; i < materials.size(); ++i)
			{
				auto& material = materials[i];
				if (!material)
				{
					DrawInfoRow(("Material " + std::to_string(i)).c_str(), "None", isHighlight);
					continue;
				}

				const std::string materialLabel = "Material " + std::to_string(i);
				DrawInfoRow(materialLabel.c_str(), "Slot", isHighlight);

				std::string currentShader = material->GetShader()
					? material->GetShader()->GetName()
					: material->GetShaderFilePath();

				BeginPropertyRow(isHighlight);
				DrawPropertyLabelCell(("Shader " + std::to_string(i)).c_str());
				PreparePropertyValueCell(GetInspectorControlYOffset());

				ImGui::PushID(static_cast<int>(i));
				ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.07f, 0.09f, 0.12f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.10f, 0.12f, 0.16f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.15f, 0.20f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 1.0f));
				ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - textPaddingX);
				if (ImGui::BeginCombo("##ShaderSelector", currentShader.c_str()))
				{
					for (const auto& shaderName : shaderNames)
					{
						const bool isSelected = (currentShader == shaderName);
						if (ImGui::Selectable(shaderName.c_str(), isSelected))
						{
							material->SetShader(shaderName);
							currentShader = shaderName;
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

				glm::vec4 baseColor = material->GetBaseColor();
				DrawColorRow(("Base Color " + std::to_string(i)).c_str(), baseColor, isHighlight);
				if (baseColor.x != material->GetBaseColor().x ||
					baseColor.y != material->GetBaseColor().y ||
					baseColor.z != material->GetBaseColor().z ||
					baseColor.w != material->GetBaseColor().w)
				{
					material->SetBaseColor(baseColor);
				}

				DrawInfoRow(("Albedo " + std::to_string(i)).c_str(), material->GetAlbedoTexturePath(), isHighlight);
			}

			EndPropertyTable();
		}

		ImGui::Indent(treeIndent);
	}

	void InspectorPanel::DrawLightComponentProperties(LightComponent& lightComponent)
	{
		const float treeIndent = ImGui::GetTreeNodeToLabelSpacing();
		ImGui::Unindent(treeIndent);

		if (BeginPropertyTable("##LightComponentTable"))
		{
			bool isHighlight = false;

			glm::vec4 lightColor = lightComponent.color;
			DrawColorRow("Color", lightColor, isHighlight);
			if (lightColor.x != lightComponent.color.x ||
				lightColor.y != lightComponent.color.y ||
				lightColor.z != lightComponent.color.z ||
				lightColor.w != lightComponent.color.w)
			{
				lightComponent.color = lightColor;
			}

			float intensity = lightComponent.intensity;
			DrawFloatRow("Intensity", intensity, isHighlight, 0.05f, 0.0f);
			if (intensity != lightComponent.intensity)
			{
				lightComponent.intensity = intensity;
			}

			EndPropertyTable();
		}

		ImGui::Indent(treeIndent);
	}

	void InspectorPanel::DrawLineRendererProperties(LineRenderer& lineRenderer, PointData& selectedPoint)
	{
		const float treeIndent = ImGui::GetTreeNodeToLabelSpacing();
		ImGui::Unindent(treeIndent);

		if (BeginPropertyTable("##LineRendererComponentTable"))
		{
			bool isHighlight = false;

			CurveType curveType = lineRenderer.GetCurveType();
			DrawCurveTypeRow("Curve Type", curveType, isHighlight);
			if (curveType != lineRenderer.GetCurveType())
			{
				lineRenderer.SetCurveType(curveType);
			}

			glm::vec4 lineColor = lineRenderer.GetLineColor();
			DrawColorRow("Line Color", lineColor, isHighlight);
			if (lineColor.x != lineRenderer.GetLineColor().x ||
				lineColor.y != lineRenderer.GetLineColor().y ||
				lineColor.z != lineRenderer.GetLineColor().z ||
				lineColor.w != lineRenderer.GetLineColor().w)
			{
				lineRenderer.SetLineColor(lineColor);
			}

			float lineWidth = lineRenderer.GetLineWidth();
			DrawFloatRow("Line Width", lineWidth, isHighlight, 0.05f, 1.0f);
			if (lineWidth != lineRenderer.GetLineWidth())
			{
				lineRenderer.SetLineWidth(lineWidth);
			}

			DrawPointCountRow(lineRenderer, selectedPoint, isHighlight);

			EndPropertyTable();
		}

		ImGui::Indent(treeIndent);
	}

	void InspectorPanel::DrawLineRendererControlPoints(LineRenderer& lineRenderer, PointData& selectedPoint)
	{
		const float treeIndent = ImGui::GetTreeNodeToLabelSpacing();
		const ImGuiTreeNodeFlags childNodeFlags =
			ImGuiTreeNodeFlags_DefaultOpen |
			ImGuiTreeNodeFlags_Framed |
			ImGuiTreeNodeFlags_SpanAvailWidth |
			ImGuiTreeNodeFlags_FramePadding;

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4,4 });
		const bool open = ImGui::TreeNodeEx("Control Points", childNodeFlags);
		ImGui::PopStyleVar();

		if (!open)
		{
			return;
		}

		ImGui::Unindent(treeIndent);

		if (BeginPropertyTable("##LineRendererControlPointsTable"))
		{
			bool isHighlight = false;

			for (uint32_t i = 0; i < lineRenderer.GetControlPointCount(); ++i)
			{
				if (!lineRenderer.IsAnchorControlPoint(static_cast<int>(i)))
				{
					continue;
				}

				auto currentPoint = lineRenderer.GetControlPointByIndex(static_cast<int>(i));
				glm::vec3 pointPosition = currentPoint.position;
				BezierHandleMode handleMode = lineRenderer.GetHandleModeForPoint(static_cast<int>(i));
				const std::string pointLabel = "Anchor " + std::to_string(i / 3);
				DrawAnchorRow(pointLabel.c_str(), pointPosition, handleMode, isHighlight);

				if (pointPosition.x != currentPoint.position.x ||
					pointPosition.y != currentPoint.position.y ||
					pointPosition.z != currentPoint.position.z)
				{
					lineRenderer.MoveControlPoint(static_cast<int>(i), pointPosition);
					if (selectedPoint.id == static_cast<int>(i))
					{
						selectedPoint = lineRenderer.GetControlPointByIndex(static_cast<int>(i));
					}
				}

				if (handleMode != lineRenderer.GetHandleModeForPoint(static_cast<int>(i)))
				{
					lineRenderer.SetHandleModeForPoint(static_cast<int>(i), handleMode);
				}
			}

			EndPropertyTable();
		}

		ImGui::Indent(treeIndent);
		ImGui::TreePop();
	}
}
