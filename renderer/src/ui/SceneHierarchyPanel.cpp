#include "renderer_pch.h"
#include "SceneHierarchyPanel.h"

#include "imgui.h"
#include <imgui_internal.h>

namespace Kita {




	const float inspectorLabelColumn_LeftWidth = 130.0f; 

	const float rowHeight = 32.0f;
	const float textPaddingX = 10.0f;

	const float separatorThickness = 1.7f;
	const ImU32 separatorColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.23f, 0.27f, 0.33f, 1.0f));

	const ImU32 tableBgColor_Dark = ImGui::ColorConvertFloat4ToU32(ImVec4(0.10f, 0.11f, 0.135f, 1.0f));
	const ImU32 tableBgColor_Light = ImGui::ColorConvertFloat4ToU32(ImVec4(0.14f, 0.15f, 0.19f, 1.0f));

	const float infoRowsGap = 3.0f;

	void SceneHierarchyPanel::SetSelectedObject(Object obj)
	{
		if (obj)
		{
			m_SelectedObject = obj;
			KITA_CLENT_INFO("{0},this object is selected", m_SelectedObject.GetComponent<Name>().Get().c_str());
		}
		else
		{
			m_SelectedObject = {};
		}
	}

	void SceneHierarchyPanel::SetSelectedPoint(PointData point)
	{

		if (point.id != -1)
		{
			m_SelectedPoint = point;
			KITA_CLENT_INFO("{0},this object is selected", m_SelectedObject.GetComponent<Name>().Get().c_str());
		}
		else
		{
			m_SelectedPoint = {};
		}
	}

	void SceneHierarchyPanel::ClearSelectedPoint()
	{
		if (m_SelectedObject && m_SelectedPoint.id != -1 && m_SelectedObject.HasComponent<LineRenderer>())
		{
			m_SelectedObject.GetComponent<LineRenderer>().ResetControlPointVisual(m_SelectedPoint.id);
		}

		m_SelectedPoint = {};
	}

	void SceneHierarchyPanel::ClearSelection()
	{
		ClearSelectedPoint();
		m_SelectedObject = {};
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGuiWindowClass viewportWindowClass{};
		viewportWindowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
		ImGui::SetNextWindowClass(&viewportWindowClass);
		ImGui::Begin("Hierarchy");

		if (m_SceneContext)
		{
			ImGuiIO& io = ImGui::GetIO();
			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;


			if (ImGui::IsItemClicked())
			{
				m_SelectedObject = {};
			}

			auto view = m_SceneContext->GetRegistry().view<entt::entity>();
			for (auto entityID : view)
			{
				Object obj{ entityID, m_SceneContext.get(), "" };
				DrawObjectNode(obj);
			}



			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
			{
				ClearSelection();
			}

			if (!ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			{

				ImGui::OpenPopup("hierarchy_context_menu");
			}

			if (ImGui::BeginPopup("hierarchy_context_menu")) {
				if (ImGui::MenuItem("Create Empty Object")) { m_SceneContext->CreateObject("Empty Object"); }
				if (ImGui::MenuItem("Create Quad ")) { m_SceneContext->CreateObject("Quad Object"); }
				ImGui::EndPopup();
			}
		}

		ImGui::End();
		DrawInspectorPanel();
	}

	void SceneHierarchyPanel::DrawObjectNode(Object obj)
	{
		auto& name = obj.GetName();
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ((m_SelectedObject == obj) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_SpanAvailWidth;

		bool open = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)obj, flags, name.c_str());

		if (ImGui::IsItemClicked())
		{
			ClearSelectedPoint();
			SetSelectedObject(obj);
		}

		bool deleted = false;
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete"))
			{
				deleted = true;
			}
			ImGui::EndPopup();
		}

		if (open)
		{

			ImGui::TreePop();
		}
		if (deleted)
		{
			m_SceneContext->DestroyObject(obj);
			if (m_SelectedObject == obj)
			{
				m_SelectedObject = {};
			}
		}
	}


	template<typename T, typename Func>
	static void DrawComponent(const std::string& label, Object object, Func func, bool enableRemove = true)
	{
		const ImGuiTreeNodeFlags componentNodeFlags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_AllowOverlap
			| ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
		const std::string moreOperation_Str = "Component More Operation";
		if (object.HasComponent<T>())
		{
			auto& component = object.GetComponent<T>();
			ImVec2 contentAvailabel = ImGui::GetContentRegionAvail();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4,4 });
			float lineHeight = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImGui::Separator();
			bool open = ImGui::TreeNodeEx((void*)typeid(T).hash_code(), componentNodeFlags, label.c_str());
			ImGui::PopStyleVar();

			ImGui::SameLine(contentAvailabel.x - lineHeight * 0.5f);
			if (ImGui::Button("+", ImVec2{ lineHeight,lineHeight }))
			{
				ImGui::OpenPopup(moreOperation_Str.c_str());
			}

			bool remove = false;

			if (ImGui::BeginPopup(moreOperation_Str.c_str()))
			{
				if (ImGui::MenuItem("Remove Component", 0, false, enableRemove))
				{
					remove = true;
				}
				ImGui::EndPopup();
			}

			if (open)
			{
				func(component);
				ImGui::TreePop();
			}

			if (remove)
			{
				object.RemoveComponent<T>();
			}
		}
	}

	static void DrawInspectorInfoRow(
		const char* label,
		const std::string& value,
		const ImVec2& tableStartPos,
		float inspectorLabelColumnWidth,
		bool& isHightLight
	)
	{
		
		ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
	
		const ImU32 bgColor = isHightLight ? tableBgColor_Light : tableBgColor_Dark;
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bgColor);
		isHightLight = !isHightLight;

		const float textHeight = ImGui::GetTextLineHeight();
		const float contentHeight = rowHeight - ImGui::GetStyle().CellPadding.y * 2.0f;
		const float yOffset = ImMax(0.0f, (contentHeight - textHeight) * 0.5f);

		ImGui::TableSetColumnIndex(0);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yOffset);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPaddingX);
		ImGui::TextUnformatted(label);
		ImGui::TableSetColumnIndex(1);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + yOffset);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPaddingX);
		ImGui::TextUnformatted(value.c_str());

		const ImVec2 tableMin = ImGui::GetItemRectMin();
		const ImVec2 tableMax = ImGui::GetItemRectMax();
		const float separatorX = tableStartPos.x + inspectorLabelColumnWidth;
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(separatorX, tableMin.y - 3.0f),
			ImVec2(separatorX, tableMax.y + 3.0f),
			separatorColor,
			separatorThickness);
	}

	static void DrawInspectorVec3Row(
		const char* label,
		glm::vec3& value,
		const ImVec2& tableStartPos,
		float inspectorLabelColumnWidth,
		bool& isHightLight,
		float speed = 0.05f)
	{
		const float itemSpacingX = 12.0f;

		ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
		const ImU32 bgColor = isHightLight ? tableBgColor_Light : tableBgColor_Dark;
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bgColor);
		isHightLight = !isHightLight;

		const float textHeight = ImGui::GetTextLineHeight();
		const float contentHeight = rowHeight - ImGui::GetStyle().CellPadding.y * 2.0f;
		const float labelYOffset = ImMax(0.0f, (contentHeight - textHeight) * 0.5f);

		ImGui::TableSetColumnIndex(0);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + labelYOffset);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPaddingX);
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPaddingX);

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
			ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.07f, 0.09f, 0.12f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.10f, 0.12f, 0.16f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.15f, 0.20f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 1.0f));
			ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
			ImGui::SetNextItemWidth(itemWidth);
			ImGui::DragFloat(axisIds[i], (&value.x) + i, speed, 0.0f, 0.0f, "%.3f");
			const ImVec2 itemMin = ImGui::GetItemRectMin();
			const ImVec2 itemMax = ImGui::GetItemRectMax();
			ImDrawList* drawList = ImGui::GetWindowDrawList();
			const float markerRight = itemMin.x - 2.0f;
			const float markerLeft = markerRight - 4.4f;
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
			ImGui::PopStyleVar(2);
			ImGui::PopStyleColor(4);

			if (i < 2)
				ImGui::SameLine(0.0f, itemSpacingX);
		}
		ImGui::PopID();

		const ImVec2 tableMin = ImGui::GetItemRectMin();
		const ImVec2 tableMax = ImGui::GetItemRectMax();
		const float separatorX = tableStartPos.x + inspectorLabelColumnWidth;
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(separatorX, tableMin.y - 3.0f),
			ImVec2(separatorX, tableMax.y + 3.0f),
			separatorColor,
			separatorThickness);
	}

	static void DrawInspectorFloatRow(
		const char* label,
		float& value,
		const ImVec2& tableStartPos,
		float inspectorLabelColumnWidth,
		bool& isHightLight,
		float speed = 0.05f,
		float minValue = 0.0f)
	{
		ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
		const ImU32 bgColor = isHightLight ? tableBgColor_Light : tableBgColor_Dark;
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bgColor);
		isHightLight = !isHightLight;

		const float textHeight = ImGui::GetTextLineHeight();
		const float contentHeight = rowHeight - ImGui::GetStyle().CellPadding.y * 2.0f;
		const float labelYOffset = ImMax(0.0f, (contentHeight - textHeight) * 0.5f);

		ImGui::TableSetColumnIndex(0);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + labelYOffset);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPaddingX);
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPaddingX);

		ImGui::PushID(label);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.07f, 0.09f, 0.12f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.10f, 0.12f, 0.16f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.15f, 0.20f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - textPaddingX);
		ImGui::DragFloat("##FloatValue", &value, speed, minValue, 0.0f, "%.3f");
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(4);
		ImGui::PopID();

		const ImVec2 tableMin = ImGui::GetItemRectMin();
		const ImVec2 tableMax = ImGui::GetItemRectMax();
		const float separatorX = tableStartPos.x + inspectorLabelColumnWidth;
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(separatorX, tableMin.y - 3.0f),
			ImVec2(separatorX, tableMax.y + 3.0f),
			separatorColor,
			separatorThickness);
	}

	static void DrawInspectorColorRow(
		const char* label,
		glm::vec4& value,
		const ImVec2& tableStartPos,
		float inspectorLabelColumnWidth,
		bool& isHightLight)
	{
		ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
		const ImU32 bgColor = isHightLight ? tableBgColor_Light : tableBgColor_Dark;
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bgColor);
		isHightLight = !isHightLight;

		const float textHeight = ImGui::GetTextLineHeight();
		const float contentHeight = rowHeight - ImGui::GetStyle().CellPadding.y * 2.0f;
		const float labelYOffset = ImMax(0.0f, (contentHeight - textHeight) * 0.5f);

		ImGui::TableSetColumnIndex(0);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + labelYOffset);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPaddingX);
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3.0f);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPaddingX);

		ImGui::PushID(label);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 1.0f));
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - textPaddingX);
		ImGui::ColorEdit4("##ColorValue", &value.x,
			ImGuiColorEditFlags_DisplayRGB |
			ImGuiColorEditFlags_Float |
			ImGuiColorEditFlags_AlphaBar);
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
		ImGui::PopID();

		const ImVec2 tableMin = ImGui::GetItemRectMin();
		const ImVec2 tableMax = ImGui::GetItemRectMax();
		const float separatorX = tableStartPos.x + inspectorLabelColumnWidth;
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(separatorX, tableMin.y - 3.0f),
			ImVec2(separatorX, tableMax.y + 3.0f),
			separatorColor,
			separatorThickness);
	}

	static void DrawInspectorCurveTypeRow(
		const char* label,
		CurveType& curveType,
		const ImVec2& tableStartPos,
		float inspectorLabelColumnWidth,
		bool& isHightLight)
	{
		ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
		const ImU32 bgColor = isHightLight ? tableBgColor_Light : tableBgColor_Dark;
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bgColor);
		isHightLight = !isHightLight;

		const float textHeight = ImGui::GetTextLineHeight();
		const float contentHeight = rowHeight - ImGui::GetStyle().CellPadding.y * 2.0f;
		const float labelYOffset = ImMax(0.0f, (contentHeight - textHeight) * 0.5f);

		ImGui::TableSetColumnIndex(0);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + labelYOffset);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPaddingX);
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPaddingX);

		const char* items[] = { "Polyline", "BezierCubic" };
		int currentIndex = static_cast<int>(curveType);

		ImGui::PushID(label);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.07f, 0.09f, 0.12f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.10f, 0.12f, 0.16f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.15f, 0.20f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - textPaddingX);
		if (ImGui::Combo("##CurveType", &currentIndex, items, IM_ARRAYSIZE(items)))
		{
			curveType = static_cast<CurveType>(currentIndex);
		}
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(4);
		ImGui::PopID();

		const ImVec2 tableMin = ImGui::GetItemRectMin();
		const ImVec2 tableMax = ImGui::GetItemRectMax();
		const float separatorX = tableStartPos.x + inspectorLabelColumnWidth;
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(separatorX, tableMin.y - 3.0f),
			ImVec2(separatorX, tableMax.y + 3.0f),
			separatorColor,
			separatorThickness);
	}

	static void DrawInspectorHandleModeRow(
		const char* label,
		BezierHandleMode& handleMode,
		const ImVec2& tableStartPos,
		float inspectorLabelColumnWidth,
		bool& isHightLight)
	{
		ImGui::TableNextRow(ImGuiTableRowFlags_None, rowHeight);
		const ImU32 bgColor = isHightLight ? tableBgColor_Light : tableBgColor_Dark;
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, bgColor);
		isHightLight = !isHightLight;

		const float textHeight = ImGui::GetTextLineHeight();
		const float contentHeight = rowHeight - ImGui::GetStyle().CellPadding.y * 2.0f;
		const float labelYOffset = ImMax(0.0f, (contentHeight - textHeight) * 0.5f);

		ImGui::TableSetColumnIndex(0);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + labelYOffset);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPaddingX);
		ImGui::TextUnformatted(label);

		ImGui::TableSetColumnIndex(1);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 2.0f);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + textPaddingX);

		const char* items[] = { "Free", "Aligned", "Mirrored" };
		int currentIndex = static_cast<int>(handleMode);

		ImGui::PushID(label);
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.07f, 0.09f, 0.12f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.10f, 0.12f, 0.16f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.12f, 0.15f, 0.20f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.24f, 0.30f, 1.0f));
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 3.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x - textPaddingX);
		if (ImGui::Combo("##HandleMode", &currentIndex, items, IM_ARRAYSIZE(items)))
		{
			handleMode = static_cast<BezierHandleMode>(currentIndex);
		}
		ImGui::PopStyleVar(2);
		ImGui::PopStyleColor(4);
		ImGui::PopID();

		const ImVec2 tableMin = ImGui::GetItemRectMin();
		const ImVec2 tableMax = ImGui::GetItemRectMax();
		const float separatorX = tableStartPos.x + inspectorLabelColumnWidth;
		ImGui::GetWindowDrawList()->AddLine(
			ImVec2(separatorX, tableMin.y - 3.0f),
			ImVec2(separatorX, tableMax.y + 3.0f),
			separatorColor,
			separatorThickness);
	}

	static const char* ObjectTypeToString(Type type)
	{
		switch (type)
		{
		case Type::StaticMesh: return "StaticMesh";
		case Type::Curve:      return "Curve";
		default:               return "Unknown";
		}
	}



	void SceneHierarchyPanel::DrawInspectorPanel()
	{

		ImGuiWindowClass viewportWindowClass{};
		viewportWindowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
		ImGui::SetNextWindowClass(&viewportWindowClass);

		ImGui::Begin("Inspector");
		if (m_SelectedObject)
		{

#pragma region  ------------------------------------------------- Draw Name Component ---------------------------------------------------------

			const ImGuiTableFlags infoTableFlags =
				ImGuiTableFlags_SizingFixedFit |
				ImGuiTableFlags_NoSavedSettings;

			if (ImGui::BeginTable("##InspectorObjectInfoTable", 2, infoTableFlags))
			{
				bool isHightLight = false;
				const ImVec2 tableStartPos = ImGui::GetCursorScreenPos();
				ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, inspectorLabelColumn_LeftWidth);
				ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

				if (m_SelectedObject.HasComponent<Name>())
				{
					auto& name = m_SelectedObject.GetComponent<Name>().Get();
					DrawInspectorInfoRow("Name", name, tableStartPos, inspectorLabelColumn_LeftWidth,isHightLight);
				}
				ImGui::TableNextRow(ImGuiTableRowFlags_None, infoRowsGap);
				if (m_SelectedObject.HasComponent<ObjectType>())
				{
					const Type& type = m_SelectedObject.GetComponent<ObjectType>().Get();
					const std::string typeText = ObjectTypeToString(type);
					DrawInspectorInfoRow("Type",typeText,tableStartPos,inspectorLabelColumn_LeftWidth, isHightLight);
				}
				ImGui::TableNextRow(ImGuiTableRowFlags_None, infoRowsGap);
				if (m_SelectedObject.HasComponent<Transform>())
				{
					auto& transform = m_SelectedObject.GetComponent<Transform>();
					DrawInspectorVec3Row("Position", transform.GetPosition(), tableStartPos, inspectorLabelColumn_LeftWidth, isHightLight);
					DrawInspectorVec3Row("Rotation", transform.GetRotation(), tableStartPos, inspectorLabelColumn_LeftWidth, isHightLight);
					DrawInspectorVec3Row("Scale", transform.GetScale(), tableStartPos, inspectorLabelColumn_LeftWidth, isHightLight);
				}

				ImGui::EndTable();
			}

			DrawComponent<LineRenderer>("LineRenderer", m_SelectedObject, [&](LineRenderer& lineRenderer)
			{
				const float treeIndent = ImGui::GetTreeNodeToLabelSpacing();
				ImGui::Unindent(treeIndent);

				const ImGuiTableFlags componentTableFlags =
					ImGuiTableFlags_SizingFixedFit |
					ImGuiTableFlags_NoSavedSettings;

				if (ImGui::BeginTable("##LineRendererComponentTable", 2, componentTableFlags))
				{
					bool isHightLight = false;
					const ImVec2 tableStartPos = ImGui::GetCursorScreenPos();
					ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, inspectorLabelColumn_LeftWidth);
					ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

					CurveType curveType = lineRenderer.GetCurveType();
					DrawInspectorCurveTypeRow("Curve Type", curveType, tableStartPos, inspectorLabelColumn_LeftWidth, isHightLight);
					if (curveType != lineRenderer.GetCurveType())
					{
						lineRenderer.SetCurveType(curveType);
					}

					glm::vec4 lineColor = lineRenderer.GetLineColor();
					DrawInspectorColorRow("Line Color", lineColor, tableStartPos, inspectorLabelColumn_LeftWidth, isHightLight);
					if (lineColor.x != lineRenderer.GetLineColor().x ||
						lineColor.y != lineRenderer.GetLineColor().y ||
						lineColor.z != lineRenderer.GetLineColor().z ||
						lineColor.w != lineRenderer.GetLineColor().w)
					{
						lineRenderer.SetLineColor(lineColor);
					}

					float lineWidth = lineRenderer.GetLineWidth();
					DrawInspectorFloatRow("Line Width", lineWidth, tableStartPos, inspectorLabelColumn_LeftWidth, isHightLight, 0.05f, 1.0f);
					if (lineWidth != lineRenderer.GetLineWidth())
					{
						lineRenderer.SetLineWidth(lineWidth);
					}

					DrawInspectorInfoRow(
						"Point Count",
						std::to_string(lineRenderer.GetControlPointCount()),
						tableStartPos,
						inspectorLabelColumn_LeftWidth,
						isHightLight);

					DrawInspectorInfoRow(
						"Segment Count",
						std::to_string(lineRenderer.GetBezierSegmentCount()),
						tableStartPos,
						inspectorLabelColumn_LeftWidth,
						isHightLight);

					if (m_SelectedPoint.id != -1)
					{
						BezierHandleMode handleMode = lineRenderer.GetHandleModeForPoint(m_SelectedPoint.id);
						DrawInspectorHandleModeRow("Handle Mode", handleMode, tableStartPos, inspectorLabelColumn_LeftWidth, isHightLight);
						if (handleMode != lineRenderer.GetHandleModeForPoint(m_SelectedPoint.id))
						{
							lineRenderer.SetHandleModeForPoint(m_SelectedPoint.id, handleMode);
						}
					}

					ImGui::EndTable();
				}

				if (lineRenderer.GetCurveType() == CurveType::BezierCubic)
				{
					if (ImGui::Button("Add Segment"))
					{
						lineRenderer.AppendBezierSegment();
					}
					ImGui::SameLine();
					if (ImGui::Button("Remove Segment"))
					{
						lineRenderer.RemoveLastBezierSegment();
						if (m_SelectedPoint.id >= static_cast<int>(lineRenderer.GetControlPointCount()))
						{
							m_SelectedPoint = {};
						}
					}
				}

				const ImGuiTreeNodeFlags childNodeFlags =
					ImGuiTreeNodeFlags_DefaultOpen |
					ImGuiTreeNodeFlags_Framed |
					ImGuiTreeNodeFlags_SpanAvailWidth |
					ImGuiTreeNodeFlags_FramePadding;

				ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4,4 });
				const bool open = ImGui::TreeNodeEx("Control Points", childNodeFlags);
				ImGui::PopStyleVar();

				if (open)
				{
					ImGui::Unindent(treeIndent);

					const ImGuiTableFlags pointTableFlags =
						ImGuiTableFlags_SizingFixedFit |
						ImGuiTableFlags_NoSavedSettings;

					if (ImGui::BeginTable("##LineRendererControlPointsTable", 2, pointTableFlags))
					{
						bool pointHighlight = false;
						const ImVec2 pointTableStartPos = ImGui::GetCursorScreenPos();
						ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthFixed, inspectorLabelColumn_LeftWidth);
						ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthStretch);

						for (uint32_t i = 0; i < lineRenderer.GetControlPointCount(); ++i)
						{
							auto currentPoint = lineRenderer.GetControlPointByIndex(static_cast<int>(i));
							glm::vec3 pointPosition = currentPoint.position;
							std::string pointLabel;
							if (lineRenderer.IsAnchorControlPoint(static_cast<int>(i)))
							{
								pointLabel = "Anchor " + std::to_string(i / 3);
							}
							else if (static_cast<int>(i) % 3 == 1)
							{
								pointLabel = "Out Handle " + std::to_string(i / 3);
							}
							else
							{
								pointLabel = "In Handle " + std::to_string((i + 1) / 3);
							}
							DrawInspectorVec3Row(
								pointLabel.c_str(),
								pointPosition,
								pointTableStartPos,
								inspectorLabelColumn_LeftWidth,
								pointHighlight);

							if (pointPosition.x != currentPoint.position.x ||
								pointPosition.y != currentPoint.position.y ||
								pointPosition.z != currentPoint.position.z)
							{
								lineRenderer.MoveControlPoint(static_cast<int>(i), pointPosition);
								if (m_SelectedPoint.id == static_cast<int>(i))
								{
									m_SelectedPoint = lineRenderer.GetControlPointByIndex(static_cast<int>(i));
								}
							}
						}

						ImGui::EndTable();
					}

					ImGui::Indent(treeIndent);
					ImGui::TreePop();
				}

				ImGui::Indent(treeIndent);
			});

#pragma endregion

		}

		ImGui::End();
	}

	void SceneHierarchyPanel::OnSlectedObjectChange(Object obj)
	{
		m_SelectedObject = obj;
	}

}

