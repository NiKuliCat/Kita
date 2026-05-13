#include "renderer_pch.h"
#include "SceneHierarchyPanel.h"

#include "imgui.h"
#include <imgui_internal.h>

namespace Kita {
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
			auto view = m_SceneContext->GetRegistry().view<entt::entity>();
			for (auto entityID : view)
			{
				Object obj{ entityID, m_SceneContext.get(), "" };
				DrawObjectNode(obj);
			}


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

			if (ImGui::BeginPopup("hierarchy_context_menu")) {
				if (ImGui::MenuItem("Create Empty Object")) { m_SceneContext->CreateObject("Empty Object"); }
				if (ImGui::MenuItem("Create Quad ")) { m_SceneContext->CreateObject("Quad Object"); }
				ImGui::EndPopup();
			}
		}

		ImGui::End();
	}

	void SceneHierarchyPanel::DrawObjectNode(Object obj)
	{
		auto& name = obj.GetName();
		const bool isSelected =
			m_SelectionContext &&
			m_SelectionContext->GetSelectionType() == EditorSelectionItemType::SceneObject &&
			m_SelectionContext->GetSelectionItemHandle().m_SelectionObject == obj;
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow |
			(isSelected ? ImGuiTreeNodeFlags_Selected : 0) |
			ImGuiTreeNodeFlags_SpanAvailWidth;

		bool open = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)obj, flags, name.c_str());

		if (ImGui::IsItemClicked())
		{
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
	}

	void SceneHierarchyPanel::OnSlectedObjectChange(Object obj)
	{
		SetSelectedObject(obj);
	}

}

