#include "renderer_pch.h"
#include "SceneHierarchyPanel.h"

#include "imgui.h"
#include <imgui_internal.h>

namespace Kita {

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
				ClearSelection();
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
	}

	void SceneHierarchyPanel::DrawObjectNode(Object obj)
	{
		auto& name = obj.GetName();
		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ((GetSelectedObject() == obj) ? ImGuiTreeNodeFlags_Selected : 0) | ImGuiTreeNodeFlags_SpanAvailWidth;

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
			if (GetSelectedObject() == obj)
			{
				ClearSelection();
			}
		}
	}

	void SceneHierarchyPanel::OnSlectedObjectChange(Object obj)
	{
		SetSelectedObject(obj);
	}

}

