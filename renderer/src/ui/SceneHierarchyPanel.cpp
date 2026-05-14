#include "renderer_pch.h"
#include "SceneHierarchyPanel.h"

#include "imgui.h"
#include <imgui_internal.h>

namespace Kita {
	namespace
	{
		constexpr float hierarchyHeaderCellPaddingY = 4.0f;
		constexpr float hierarchyRowCellPaddingY = 3.0f;
		constexpr float hierarchyHeaderRowHeight = 25.0f;
		constexpr float hierarchyRowHeight = 24.0f;
		constexpr float hierarchyVisibilityColumnWidth = 34.0f;
		constexpr float hierarchyDirtyColumnWidth = 34.0f;
		constexpr float hierarchyTypeColumnWidth = 148.0f;
		constexpr const char* hierarchySearchIconName = "icons8-search-104";

		const ImVec4 hierarchyToolbarBg = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
		const ImVec4 hierarchyToolbarButtonHovered = ImVec4(0.26f, 0.26f, 0.26f, 1.0f);
		const ImVec4 hierarchyToolbarButtonActive = ImVec4(0.18f, 0.18f, 0.18f, 1.0f);
		const ImVec4 hierarchyHeaderBg = ImVec4(0.17f, 0.17f, 0.17f, 1.0f);
		const ImVec4 hierarchyRowBg = ImVec4(0.10f, 0.10f, 0.10f, 1.0f);
		const ImVec4 hierarchyRowAltBg = ImVec4(0.12f, 0.12f, 0.12f, 1.0f);
		const ImVec4 hierarchyBorder = ImVec4(0.06f, 0.06f, 0.06f, 1.0f);
		const ImVec4 hierarchyMutedText = ImVec4(0.62f, 0.62f, 0.62f, 1.0f);
		const ImVec4 hierarchyTypeText = ImVec4(0.58f, 0.58f, 0.58f, 1.0f);
		const ImVec4 hierarchyHoveredRowBg = ImVec4(0.33f, 0.33f, 0.33f, 1.0f);
		const ImVec4 hierarchySelectedRowBg = ImVec4(0.17f, 0.36f, 0.72f, 1.0f);
	}

	namespace
	{
		void DrawCenteredHeaderLabel(const char* label)
		{
			const ImVec2 textSize = ImGui::CalcTextSize(label);
			const float columnWidth = ImGui::GetColumnWidth();
			const float cursorX = ImGui::GetCursorPosX();
			const float cursorY = ImGui::GetCursorPosY();
			const float offsetX = ImMax(0.0f, (columnWidth - textSize.x) * 0.5f);
			const float offsetY = ImMax(0.0f, (hierarchyHeaderRowHeight - textSize.y) * 0.5f - ImGui::GetStyle().CellPadding.y);
			ImGui::SetCursorPosX(cursorX + offsetX);
			ImGui::SetCursorPosY(cursorY + offsetY);
			ImGui::TextUnformatted(label);
		}
	}

	const char* SceneHierarchyPanel::ObjectTypeToString(Type type)
	{
		switch (type)
		{
		case Type::StaticMesh: return "StaticMesh";
		case Type::Curve:      return "Curve";
		default:               return "Unknown";
		}
	}

	void SceneHierarchyPanel::SetSelectedObject(Object obj)
	{
		if (m_SelectionContext)
		{
			m_SelectionContext->SetSelectionType(EditorSelectionItemType::SceneObject);
			m_SelectionContext->SetSelctionObject(obj);
		}
	}

	void SceneHierarchyPanel::OnImGuiRender()
	{
		ImGuiWindowClass viewportWindowClass{};
		viewportWindowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
		ImGui::SetNextWindowClass(&viewportWindowClass);
		ImGui::Begin("Hierarchy");

		if (m_SceneContext)
		{
			DrawToolbar();
			DrawObjectList();

			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
			{
				if (m_SelectionContext)
				{
					m_SelectionContext->Clear();
				}
			}

			if (!ImGui::IsAnyItemHovered() && ImGui::IsWindowHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right))
			{
				ImGui::OpenPopup("hierarchy_context_menu");
			}

			if (ImGui::BeginPopup("hierarchy_context_menu"))
			{
				if (ImGui::MenuItem("Create Empty Object")) { m_SceneContext->CreateObject("Empty Object"); }
				if (ImGui::MenuItem("Create Quad ")) { m_SceneContext->CreateObject("Quad Object"); }
				ImGui::EndPopup();
			}
		}

		ImGui::End();
	}

	void SceneHierarchyPanel::DrawToolbar()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.5f);
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(30.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.06f, 0.06f, 0.06f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.06f, 0.06f, 0.06f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.06f, 0.06f, 0.06f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_Border, hierarchyBorder);

		ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::InputTextWithHint("##HierarchySearch", "Search...", m_SearchBuffer.data(), m_SearchBuffer.size());
		// TODO: hook up hierarchy search/filter behavior.

		const ImVec2 inputMin = ImGui::GetItemRectMin();
		const ImVec2 inputMax = ImGui::GetItemRectMax();

		const SvgIconAtlas::IconHandle searchIcon =
			m_IconAtlas && m_IconAtlas->IsLoaded() ? m_IconAtlas->FindIcon(hierarchySearchIconName) : SvgIconAtlas::IconHandle{};
		if (searchIcon.IsValid())
		{
			const float iconSize = 14.0f;
			ImGui::SetCursorScreenPos(ImVec2(
				inputMin.x + 8.0f,
				inputMin.y + (inputMax.y - inputMin.y - iconSize) * 0.5f));
			DrawAtlasIcon(searchIcon, iconSize);
			ImGui::SetCursorScreenPos(ImVec2(inputMin.x, inputMax.y));
		}

		ImGui::PopStyleColor(4);
		ImGui::PopStyleVar(3);
		ImGui::Dummy(ImVec2(0.0f, 2.0f));
	}

	void SceneHierarchyPanel::DrawObjectList()
	{
		const ImGuiTableFlags tableFlags =
			ImGuiTableFlags_RowBg |
			ImGuiTableFlags_BordersOuter |
			ImGuiTableFlags_BordersInnerV |
			ImGuiTableFlags_ScrollY |
			ImGuiTableFlags_SizingStretchProp |
			ImGuiTableFlags_NoSavedSettings;

		ImGui::PushStyleColor(ImGuiCol_TableHeaderBg, hierarchyHeaderBg);
		ImGui::PushStyleColor(ImGuiCol_TableBorderStrong, hierarchyBorder);
		ImGui::PushStyleColor(ImGuiCol_TableBorderLight, hierarchyBorder);
		ImGui::PushStyleColor(ImGuiCol_TableRowBg, hierarchyRowBg);
		ImGui::PushStyleColor(ImGuiCol_TableRowBgAlt, hierarchyRowAltBg);

		const float windowPaddingX = ImGui::GetStyle().WindowPadding.x;
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() - windowPaddingX);
		const ImVec2 tableOuterSize(ImGui::GetContentRegionAvail().x + windowPaddingX * 2.0f, 0.0f);

		if (ImGui::BeginTable("##HierarchyObjectsTable", 4, tableFlags, tableOuterSize))
		{
			ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, hierarchyVisibilityColumnWidth);
			ImGui::TableSetupColumn("*", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, hierarchyDirtyColumnWidth);
			ImGui::TableSetupColumn("Label", ImGuiTableColumnFlags_WidthStretch | ImGuiTableColumnFlags_NoResize);
			ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize, hierarchyTypeColumnWidth);

			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, hierarchyHeaderCellPaddingY));
			ImGui::TableNextRow(ImGuiTableRowFlags_Headers, hierarchyHeaderRowHeight);
			ImGui::TableSetColumnIndex(0);
			DrawCenteredHeaderLabel("");
			ImGui::TableSetColumnIndex(1);
			DrawCenteredHeaderLabel("*");
			ImGui::TableSetColumnIndex(2);
			DrawCenteredHeaderLabel("Label");
			ImGui::TableSetColumnIndex(3);
			DrawCenteredHeaderLabel("Type");
			ImGui::PopStyleVar();

			ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(4.0f, hierarchyRowCellPaddingY));
			auto view = m_SceneContext->GetRegistry().view<entt::entity>();
			for (auto entityID : view)
			{
				Object obj{ entityID, m_SceneContext.get(), "" };
				DrawObjectNode(obj);
			}

			if (m_PendingDeleteObject != entt::null)
			{
				Object objectToDelete{ m_PendingDeleteObject, m_SceneContext.get(), "" };
				m_SceneContext->DestroyObject(objectToDelete);
				m_PendingDeleteObject = entt::null;
			}
			ImGui::PopStyleVar();

			ImGui::EndTable();
		}

		ImGui::PopStyleColor(5);
	}

	void SceneHierarchyPanel::DrawAtlasIcon(const SvgIconAtlas::IconHandle& icon, float size, const ImVec4& tint)
	{
		if (!icon.IsValid())
		{
			return;
		}

		(void)tint;
		ImGui::Image(icon.TextureID, ImVec2(size, size), icon.UV0, icon.UV1);
	}

	void SceneHierarchyPanel::DrawObjectNode(Object obj)
	{
		auto& name = obj.GetName();
		const Type type = obj.HasComponent<ObjectType>() ? obj.GetComponent<ObjectType>().Get() : Type::StaticMesh;
		ImGuiTable* table = ImGui::GetCurrentTable();

		const bool isSelected =
			m_SelectionContext &&
			m_SelectionContext->GetSelectionType() == EditorSelectionItemType::SceneObject &&
			m_SelectionContext->GetSelectionItemHandle().m_SelectionObject == obj;

		ImGui::TableNextRow(ImGuiTableRowFlags_None, hierarchyRowHeight);
		const float rowMinY = table ? table->RowPosY1 : ImGui::GetCursorScreenPos().y;
		const float rowMaxY = table ? table->RowPosY2 : (rowMinY + hierarchyRowHeight);
		const float rowHeight = rowMaxY - rowMinY;
		const float textOffsetY = ImMax(0.0f, (rowHeight - ImGui::GetTextLineHeight()) * 0.5f);
		if (isSelected)
		{
			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(hierarchySelectedRowBg));
		}
		ImGui::PushID(static_cast<int>(static_cast<uint32_t>(obj)));

		ImGui::TableSetColumnIndex(0);
		ImGui::Dummy(ImVec2(12.0f, 12.0f));

		ImGui::TableSetColumnIndex(1);
		const ImVec2 dirtyCellStart = ImGui::GetCursorScreenPos();
		ImGui::SetCursorScreenPos(ImVec2(dirtyCellStart.x, rowMinY + textOffsetY));
		ImGui::PushStyleColor(ImGuiCol_Text, hierarchyMutedText);
		ImGui::TextUnformatted(" ");
		ImGui::PopStyleColor();

		ImGui::TableSetColumnIndex(2);
		const ImVec2 labelCellStart = ImGui::GetCursorScreenPos();
		const ImVec2 rowStart(labelCellStart.x, rowMinY);
		ImGui::SetCursorScreenPos(rowStart);
		ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0, 0, 0, 0));
		ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0, 0, 0, 0));
		ImGui::Selectable("##HierarchyRowSelect", false, ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowOverlap, ImVec2(0.0f, rowHeight));
		const bool isHovered = ImGui::IsItemHovered();
		ImGui::PopStyleColor(3);
		if (!isSelected && isHovered)
		{
			ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, ImGui::GetColorU32(hierarchyHoveredRowBg));
		}
		if (ImGui::IsItemClicked())
		{
			SetSelectedObject(obj);
		}

		ImGui::SetCursorScreenPos(ImVec2(labelCellStart.x + 8.0f, rowMinY + textOffsetY));
		ImGui::PushStyleColor(ImGuiCol_Text, hierarchyMutedText);
		ImGui::TextUnformatted(" ");
		ImGui::PopStyleColor();
		// TODO: hook up hierarchical expand/collapse when parent-child data exists.
		ImGui::SameLine(0.0f, 6.0f);
		ImGui::TextUnformatted(name.c_str());

		ImGui::TableSetColumnIndex(3);
		ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x, rowMinY + textOffsetY));
		ImGui::PushStyleColor(ImGuiCol_Text, hierarchyTypeText);
		ImGui::TextUnformatted(ObjectTypeToString(type));
		ImGui::PopStyleColor();

		bool deleted = false;
		if (ImGui::BeginPopupContextItem("HierarchyObjectContext"))
		{
			if (ImGui::MenuItem("Delete"))
			{
				deleted = true;
			}
			ImGui::EndPopup();
		}

		if (deleted)
		{
			m_PendingDeleteObject = static_cast<entt::entity>(obj);
		}

		ImGui::PopID();
	}

	void SceneHierarchyPanel::OnSlectedObjectChange(Object obj)
	{
		SetSelectedObject(obj);
	}

}
