#include "renderer_pch.h"
#include "EditorLayer.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "utils/FileDialogs.h"
#include "file/Project.h"

#include <glm/glm.hpp>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

namespace Kita {

	EditorLayer::EditorLayer()
		:Layer("EditorLayer")
	{
		
	}
	void EditorLayer::OnCreate()
	{
		m_Scene = CreateRef<Scene>( "example scene");

		{
			auto obj = m_Scene->CreateObject("sphere");
			auto& meshrenderer = obj.AddComponent<MeshRenderer>();
			meshrenderer.LoadMeshs("content/models/Sphere.fbx");

			auto curveObj1 = m_Scene->CreateObject("curve 1");
			auto& lineRenderer1 = curveObj1.AddComponent<LineRenderer>();
			lineRenderer1.SetLineWidth(4.0f);
			lineRenderer1.SetLineColor({ 1,1,0,1 });

			auto curveObj2 = m_Scene->CreateObject("curve 2");
			auto& transform2 = curveObj2.GetComponent<Transform>();
			transform2.SetPosition({2,2,2});
			auto& lineRenderer2 = curveObj2.AddComponent<LineRenderer>();
			lineRenderer2.SetLineWidth(4.0f);
			lineRenderer2.SetLineColor({ 1,0,1,1 });
		}

		m_SceneSerializer = SceneSerializer(m_Scene);

		CubemapFacePaths faces = {
			"packages/skybox/right.jpg",  // +X
			"packages/skybox/left.jpg",   // -X
			"packages/skybox/top.jpg",    // +Y
			"packages/skybox/bottom.jpg", // -Y
			"packages/skybox/front.jpg",  // +Z
			"packages/skybox/back.jpg"    // -Z
		};

		m_Scene->LoadSkyCubemap(faces);

		m_SceneSelectionContext = CreateRef<SceneSelectionContext>();
		m_SceneHierarchyPanel = SceneHierarchyPanel(m_Scene, m_SceneSelectionContext);
		m_InspectorPanel = InspectorPanel(m_SceneSelectionContext);

		m_SceneViewportPanels.clear();
		m_NextViewportSerial = 1;
		AddViewportPanel("Viewport");

		m_ContentBrowserPanel = ContentBrowserPanel(Project::GetActive()->GetContentDirectory());
	}

	void EditorLayer::OnUpdate(float daltaTime)
	{
		m_Scene->SimulateSceneEditor();
		RemoveClosedViewportPanels();
		for (auto& viewport : m_SceneViewportPanels)
		{
			viewport->Simulate(daltaTime);
			viewport->Render();
		}
	}

	void EditorLayer::OnDestroy()
	{
	}

	void EditorLayer::OnImGuiRender()
	{
		static bool p_open = true;
		static bool opt_fullscreen = true;
		static bool opt_padding = false;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

		if (opt_fullscreen)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else
		{
			dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		if (!opt_padding)
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("DockSpace Demo", &p_open, window_flags);

		if (!opt_padding)
			ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec2 minWindowSize = style.WindowMinSize;

		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}
		style.WindowMinSize = minWindowSize;


		if (ImGui::BeginMenuBar())
		{

			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Save Scene"))
				{
					if (m_Scene->HasFilePath())
					{
						m_SceneSerializer.Serialize();
					}
					else
					{
						auto path = FileDialogs::SaveFile(L"Kita Scene (*.sce)\0*.sce\0All Files (*.*)\0*.*\0", L"sce");
						if (!path.empty())
						{
							m_SceneSerializer.Serialize(path);
						}
					}
				}

				if (ImGui::MenuItem("Load Scene"))
				{
					auto path = FileDialogs::OpenFile(L"Kita Scene (*.sce)\0*.sce\0All Files (*.*)\0*.*\0");
					if (!path.empty())
					{
						m_SceneHierarchyPanel.ClearSelection();
						m_SceneSerializer.Deserialize(path);
					}
				}

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Window"))
			{
				if (ImGui::MenuItem("Viewport"))
				{
					AddViewportPanel("Viewport " + std::to_string(m_NextViewportSerial++));
				}

				if (ImGui::MenuItem("UI Color Panel"))
				{
					m_UIColorPanel.SetOpen(true);
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Setting"))
			{
				if (ImGui::MenuItem("New"))
				{

				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Help"))
			{
				if (ImGui::MenuItem("New"))
				{

				}

				ImGui::EndMenu();
			}

			const std::string sceneName = m_Scene->GetName().c_str();

			ImVec2 textSize = ImGui::CalcTextSize(sceneName.c_str());

			float leftBound = ImGui::GetCursorPosX();
			float centerPosX = (ImGui::GetWindowWidth() - textSize.x) * 0.5f;
			float finalPosX = (centerPosX > leftBound) ? centerPosX : leftBound + 10.0f;

			float cursorY = ImGui::GetCursorPosY();
			ImGui::SetCursorPosX(finalPosX);
			ImGui::SetCursorPosY(cursorY);

			ImGui::TextColored(ImVec4(0.75f, 0.85f, 0.30f, 1.0f), sceneName.c_str());


			ImGui::EndMenuBar();
		}

		m_SceneHierarchyPanel.OnImGuiRender();
		m_InspectorPanel.OnImGuiRender();
		m_ContentBrowserPanel.OnImGuiRender();
		m_UIColorPanel.OnImGuiRender();

		for (size_t i = 0; i < m_SceneViewportPanels.size(); ++i)
		{
			auto& viewport = m_SceneViewportPanels[i];
			viewport->SetActive(static_cast<int32_t>(i) == m_ActiveViewportIndex);
			viewport->OnImGuiRender();

			if (viewport->IsImageHovered())
				m_ActiveViewportIndex = static_cast<int32_t>(i);
		}



		ImGui::End();


	}

	void EditorLayer::OnEvent(Event& event)
	{
		EventDisPatcher dispatcher(event);
		dispatcher.Dispatcher<KeyPressedEvent>(BIND_EVENT_FUNC(EditorLayer::OnKeyPressed));
		dispatcher.Dispatcher<MouseButtonPressedEvent>(BIND_EVENT_FUNC(EditorLayer::OnMouseButtonPressed));

		if (event.IsInCategory(EventCategory::EventMouse))
		{
			for (size_t i = 0; i < m_SceneViewportPanels.size(); ++i)
			{
				if (m_SceneViewportPanels[i]->IsImageHovered())
				{
					m_ActiveViewportIndex = static_cast<int32_t>(i);
					break;
				}
			}
		}

		for (size_t i = 0; i < m_SceneViewportPanels.size(); ++i)
		{
			auto& viewport = m_SceneViewportPanels[i];
			viewport->SetActive(static_cast<int32_t>(i) == m_ActiveViewportIndex);
			viewport->OnEvent(event);
		}

	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
	{
		return false;
	}

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& event)
	{
		return false;
	}

	void EditorLayer::AddViewportPanel(std::string windowName)
	{
		m_SceneViewportPanels.emplace_back(CreateUnique<SceneViewportPanel>(m_Scene, m_SceneSelectionContext, std::move(windowName)));
		m_ActiveViewportIndex = static_cast<int32_t>(m_SceneViewportPanels.size()) - 1;
	}

	void EditorLayer::RemoveClosedViewportPanels()
	{
		for (size_t i = 0; i < m_SceneViewportPanels.size();)
		{
			if (m_SceneViewportPanels[i]->IsOpen())
			{
				++i;
				continue;
			}

			m_SceneViewportPanels.erase(m_SceneViewportPanels.begin() + static_cast<std::ptrdiff_t>(i));

			if (m_ActiveViewportIndex > static_cast<int32_t>(i))
			{
				--m_ActiveViewportIndex;
				continue;
			}

			if (m_SceneViewportPanels.empty())
				m_ActiveViewportIndex = -1;
			else if (m_ActiveViewportIndex >= static_cast<int32_t>(m_SceneViewportPanels.size()))
				m_ActiveViewportIndex = static_cast<int32_t>(m_SceneViewportPanels.size()) - 1;
		}
	}

}

