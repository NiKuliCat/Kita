#include "renderer_pch.h"
#include "EditorLayer.h"
#include "imgui.h"

#include <glm/glm.hpp>
namespace Kita {

	void EditorLayer::OnCreate()
	{
		KITA_TRACE("{0} init", m_name.c_str());
	}

	void EditorLayer::OnUpdate(float daltaTime)
	{
	}

	void EditorLayer::OnDestroy()
	{
	}

	void EditorLayer::OnImGuiRender()
	{
		ImGui::Begin(m_name.c_str());

		ImGui::End();
	}

	void EditorLayer::OnEvent(Event& event)
	{
	}

}
