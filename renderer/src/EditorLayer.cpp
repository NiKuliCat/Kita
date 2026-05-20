#include "renderer_pch.h"
#include "EditorLayer.h"

#include "file/Project.h"
#include "project/EditorProjectBootstrap.h"
#include "utils/FileDialogs.h"

#include "imgui.h"
#include "ImGuizmo.h"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <imgui_internal.h>

namespace Kita {

	EditorLayer::EditorLayer()
		: Layer("EditorLayer")
	{
	}

	void EditorLayer::OnCreate()
	{
		EditorProjectBootstrap::Initialize();

		m_Scene = CreateRef<Scene>("example scene");
		m_SceneSerializer = SceneSerializer(m_Scene);



		m_EditorSelectionContext = CreateRef<EditorSelectionContext>();
		m_SceneHierarchyPanel = SceneHierarchyPanel(m_Scene, m_EditorSelectionContext);
		m_InspectorPanel = InspectorPanel(m_EditorSelectionContext);
		m_InspectorPanel.SetOpenAssetCallback([this](AssetHandle handle)
		{
			m_AssetEditorManager.OpenEditor(handle);
		});


		const auto project = Project::GetActive();
		if (project)
		{
			m_ContentBrowserPanel = ContentBrowserPanel(project->GetAssetRootDirectory(),m_EditorSelectionContext);
			m_EditorVulkanResourceFactory = CreateUnique<VulkanResourceFactory>(Application::Get().GetVulkanContext(), AssetManager::GetInstance());
			m_EditorVulkanResourceFactory->SetFallbackTextureHandles(
				EditorProjectBootstrap::GetPreLoadTextureHandle("white"),
				EditorProjectBootstrap::GetPreLoadTextureHandle("black"),
				EditorProjectBootstrap::GetPreLoadTextureHandle("normal"));
			m_PipelineFactory = CreateUnique<PipelineFactory>(Application::Get().GetVulkanContext());

			m_ContentBrowserThumbnailCache = CreateUnique<ThumbnailCache>(*m_EditorVulkanResourceFactory);
			m_ContentBrowserPanel.SetThumbnailCache(m_ContentBrowserThumbnailCache.get());
			m_AssetEditorManager.SetThumbnailCache(m_ContentBrowserThumbnailCache.get());
			m_AssetEditorManager.SetResourceFactory(m_EditorVulkanResourceFactory.get());
			m_ContentBrowserPanel.SetOpenAssetCallback([this](AssetHandle handle)
			{
				m_AssetEditorManager.OpenEditor(handle);
			});

			m_ContentBrowserIconAtlas = CreateUnique<SvgIconAtlas>();
			const std::filesystem::path atlasJsonPath =
				project->GetPackagesDirectory() / "editor" / "icons" / "svg" / "editor_icons_64.json";
			if (m_ContentBrowserIconAtlas->Load(atlasJsonPath))
			{
				m_ContentBrowserPanel.SetIconAtlas(m_ContentBrowserIconAtlas.get());
				m_SceneHierarchyPanel.SetIconAtlas(m_ContentBrowserIconAtlas.get());
			}
		}

		m_SceneViewportPanels.clear();
		m_NextViewportSerial = 1;
		AddViewportPanel("Viewport");

		{
			auto obj = m_Scene->CreateObject("sphere");
			auto& meshrenderer = obj.AddComponent<MeshRenderer>();
			meshrenderer.MeshAssetHandle = EditorProjectBootstrap::GetPreLoadMeshHandle("sphere");
			meshrenderer.DefaultMaterialAssetHandle = EditorProjectBootstrap::GetPreLoadMaterialHandle("default");
			meshrenderer.MaterialAssetHandles.clear();

			auto light_obj = m_Scene->CreateObject("direction light");
			auto& lightComponent =  light_obj.AddComponent<LightComponent>();
		}
	}

	void EditorLayer::OnUpdate(Timestep ts)
	{
		m_Scene->SimulateSceneEditor();
		RemoveClosedViewportPanels();
		for (auto& viewport : m_SceneViewportPanels)
			viewport.OnUpdate(ts);
	}

	void EditorLayer::OnDestroy()
	{
		m_SceneHierarchyPanel.SetIconAtlas(nullptr);
		m_ContentBrowserPanel.SetIconAtlas(nullptr);
		m_ContentBrowserPanel.SetThumbnailCache(nullptr);
		m_ContentBrowserIconAtlas.reset();
		m_ContentBrowserThumbnailCache.reset();
		if (m_EditorVulkanResourceFactory)
			m_EditorVulkanResourceFactory->Clear();
		m_EditorVulkanResourceFactory.reset();

		if (m_PipelineFactory)
			m_PipelineFactory->Clear();
		m_PipelineFactory.reset();
	}

	void EditorLayer::OnRender()
	{
		for (auto& viewport : m_SceneViewportPanels)
			viewport.OnRender();
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
			EnsureMainDockLayout(dockspace_id, ImGui::GetMainViewport()->WorkSize);
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
			m_AssetEditorManager.SetDockSpaceId(m_AssetEditorDockNodeId != 0 ? m_AssetEditorDockNodeId : dockspace_id);
		}
		else
		{
			m_MainDockLayoutInitialized = false;
			m_MainDockSpaceId = 0;
			m_AssetEditorDockNodeId = 0;
			m_AssetEditorManager.SetDockSpaceId(0);
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
							m_SceneSerializer.Serialize(path);
					}
				}

				if (ImGui::MenuItem("Load Scene"))
				{
					auto path = FileDialogs::OpenFile(L"Kita Scene (*.sce)\0*.sce\0All Files (*.*)\0*.*\0");
					if (!path.empty())
					{
						//m_SceneHierarchyPanel.ClearSelection();
						m_SceneSerializer.Deserialize(path);
					}
				}

				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Window"))
			{
				if (ImGui::MenuItem("Viewport"))
					AddViewportPanel(BuildNextViewportWindowName());

				if (ImGui::MenuItem("UI Color Panel"))
					m_UIColorPanel.SetOpen(true);

				ImGui::MenuItem("Time System", nullptr, &m_ShowTimeSystemPanel);

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
		m_AssetEditorManager.OnImGuiRender();

		for (auto& viewport : m_SceneViewportPanels)
			viewport.OnImGuiRender();


		ImGui::End();
	}

	void EditorLayer::EnsureMainDockLayout(ImGuiID dockspaceId, const ImVec2& dockspaceSize)
	{
		ImGuiDockNode* dockspaceNode = ImGui::DockBuilderGetNode(dockspaceId);
		if (m_MainDockLayoutInitialized && m_MainDockSpaceId == dockspaceId)
		{
			if (m_AssetEditorDockNodeId != 0 && ImGui::DockBuilderGetNode(m_AssetEditorDockNodeId))
			{
				return;
			}

			if (ImGuiDockNode* centralNode = ImGui::DockBuilderGetCentralNode(dockspaceId))
			{
				m_AssetEditorDockNodeId = centralNode->ID;
			}
			return;
		}

		const bool hasExistingLayout =
			dockspaceNode &&
			(dockspaceNode->IsSplitNode() || dockspaceNode->Windows.Size > 0 || dockspaceNode->CentralNode != nullptr);
		if (hasExistingLayout)
		{
			m_MainDockLayoutInitialized = true;
			m_MainDockSpaceId = dockspaceId;
			if (ImGuiDockNode* centralNode = ImGui::DockBuilderGetCentralNode(dockspaceId))
			{
				m_AssetEditorDockNodeId = centralNode->ID;
			}
			return;
		}

		ImGui::DockBuilderRemoveNode(dockspaceId);
		ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceId, dockspaceSize);

		ImGuiID hierarchyDockId = 0;
		ImGuiID inspectorDockId = 0;
		ImGuiID contentDockId = 0;
		ImGuiID centerDockId = dockspaceId;

		ImGui::DockBuilderSplitNode(centerDockId, ImGuiDir_Left, 0.18f, &hierarchyDockId, &centerDockId);
		ImGui::DockBuilderSplitNode(centerDockId, ImGuiDir_Right, 0.22f, &inspectorDockId, &centerDockId);
		ImGui::DockBuilderSplitNode(centerDockId, ImGuiDir_Down, 0.28f, &contentDockId, &centerDockId);

		ImGuiID viewportDockId = 0;
		ImGuiID assetDocumentDockId = 0;
		ImGui::DockBuilderSplitNode(centerDockId, ImGuiDir_Up, 0.62f, &viewportDockId, &assetDocumentDockId);

		ImGui::DockBuilderDockWindow("Hierarchy", hierarchyDockId);
		ImGui::DockBuilderDockWindow("Inspector", inspectorDockId);
		ImGui::DockBuilderDockWindow("Content", contentDockId);
		ImGui::DockBuilderDockWindow("Viewport", viewportDockId);
		ImGui::DockBuilderFinish(dockspaceId);

		m_MainDockLayoutInitialized = true;
		m_MainDockSpaceId = dockspaceId;
		m_AssetEditorDockNodeId = assetDocumentDockId;
		if (ImGuiDockNode* centralNode = ImGui::DockBuilderGetCentralNode(dockspaceId))
		{
			m_AssetEditorDockNodeId = centralNode->ID;
		}
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
				if (m_SceneViewportPanels[i].IsImageHovered())
				{
					m_ActiveViewportIndex = static_cast<int32_t>(i);
					break;
				}
			}
		}

		for (size_t i = 0; i < m_SceneViewportPanels.size(); ++i)
		{
			auto& viewport = m_SceneViewportPanels[i];
			viewport.SetActive(static_cast<int32_t>(i) == m_ActiveViewportIndex);
			viewport.OnEvent(event);
		}
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
	{
		if (event.IsRepeat())
		{
			return false;
		}

		switch (event.GetKeyCode())
		{
		case Key::Q:
		case Key::W:
		case Key::E:
		case Key::R:
			break;
		default:
			return false;
		}

		ImGuiIO& io = ImGui::GetIO();
		if (io.WantTextInput)
		{
			return false;
		}

		if (m_ActiveViewportIndex < 0 || m_ActiveViewportIndex >= static_cast<int32_t>(m_SceneViewportPanels.size()))
		{
			return false;
		}

		return m_SceneViewportPanels[static_cast<size_t>(m_ActiveViewportIndex)].HandleGizmoShortcut(event);
	}

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& event)
	{
		return false;
	}

	std::string EditorLayer::BuildNextViewportWindowName()
	{
		if (m_SceneViewportPanels.empty())
			return "Viewport";

		return "Viewport " + std::to_string(m_NextViewportSerial++);
	}

	void EditorLayer::AddViewportPanel(std::string windowName)
	{
		KITA_CORE_ASSERT(m_EditorVulkanResourceFactory, "EditorLayer shared VulkanResourceFactory is null");
		KITA_CORE_ASSERT(m_PipelineFactory, "EditorLayer shared PipelineFactory is null");

		m_SceneViewportPanels.emplace_back(
			Application::Get().GetVulkanContext(),
			*m_EditorVulkanResourceFactory,
			*m_PipelineFactory,
			m_Scene,
			m_EditorSelectionContext,
			std::move(windowName));
		m_ActiveViewportIndex = static_cast<int32_t>(m_SceneViewportPanels.size()) - 1;
	}

	void EditorLayer::RemoveClosedViewportPanels()
	{
		for (size_t i = 0; i < m_SceneViewportPanels.size();)
		{
			if (m_SceneViewportPanels[i].IsOpen())
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
			{
				m_ActiveViewportIndex = -1;
				m_NextViewportSerial = 1;
			}
			else if (m_ActiveViewportIndex >= static_cast<int32_t>(m_SceneViewportPanels.size()))
				m_ActiveViewportIndex = static_cast<int32_t>(m_SceneViewportPanels.size()) - 1;
		}
	}

	void EditorLayer::RenderTimeSystemPanel()
	{
		if (!m_ShowTimeSystemPanel)
			return;

		if (!ImGui::Begin("Time System", &m_ShowTimeSystemPanel))
		{
			ImGui::End();
			return;
		}

		TimeSystem& timeSystem = Application::Get().GetTimeSystem();
		const TimeSystemStatistics& stats = timeSystem.GetStatistics();
		const Timestep& timestep = timeSystem.GetTimestep();

		ImGui::Text("Frame Index: %llu", static_cast<unsigned long long>(stats.FrameIndex));
		ImGui::Separator();

		ImGui::Text("Raw Delta: %.6f s", stats.RawDeltaSeconds);
		ImGui::Text("Raw Delta: %.3f ms", stats.RawDeltaSeconds * 1000.0);
		ImGui::Text("Unscaled Delta: %.6f s", stats.UnscaleDeltaSeconds);
		ImGui::Text("Scaled Delta: %.6f s", stats.ScaledDeltaSeconds);
		ImGui::Text("Timestep Seconds: %.6f s", timestep.GetSeconds());
		ImGui::Text("Timestep Unscaled: %.6f s", timestep.GetUnscaledSeconds());
		ImGui::Separator();

		ImGui::Text("Display FPS: %.2f", stats.DisplayFPS);
		ImGui::Text("Display Delta: %.3f ms", stats.DisplayDeltaSeconds * 1000.0);
		ImGui::Text("Instant FPS: %.2f", stats.InstantFPS);
		ImGui::Text("Smooth FPS: %.2f", stats.SmoothFPS);
		ImGui::Text("Smooth Unscaled Delta: %.3f ms", stats.SmoothUnscaleDeltaSeconds * 1000.0);
		ImGui::Separator();

		ImGui::Text("Real Time: %.3f s", stats.RealTimeSeconds);
		ImGui::Text("Unscaled Time: %.3f s", stats.UnscaledTimeSeconds);
		ImGui::Text("Scaled Time: %.3f s", stats.ScaledTimeSeconds);
		ImGui::Separator();

		double timeScale = timeSystem.GetTimeScale();
		const double minTimeScale = 0.0;
		const double maxTimeScale = 4.0;
		if (ImGui::SliderScalar("Time Scale", ImGuiDataType_Double, &timeScale, &minTimeScale, &maxTimeScale, "%.2f"))
		{
			timeSystem.SetTimeScale(timeScale);
		}

		bool paused = timeSystem.IsPaused();
		if (ImGui::Checkbox("Paused", &paused))
		{
			timeSystem.SetPaused(paused);
		}

		double maxDeltaSeconds = timeSystem.GetMaxDeltaSeconds();
		const double minMaxDeltaSeconds = 0.001;
		const double maxMaxDeltaSeconds = 0.250;
		if (ImGui::SliderScalar("Max Delta (s)", ImGuiDataType_Double, &maxDeltaSeconds, &minMaxDeltaSeconds, &maxMaxDeltaSeconds, "%.3f"))
		{
			timeSystem.SetMaxDeltaSeconds(maxDeltaSeconds);
		}

		int smoothingWindowSize = static_cast<int>(timeSystem.GetSmoothingWindowSize());
		if (ImGui::SliderInt("Smoothing Window", &smoothingWindowSize, 1, 240))
		{
			timeSystem.SetSmoothingWindowSize(static_cast<std::size_t>(smoothingWindowSize));
		}

		ImGui::Text("Applied Time Scale: %.2f", stats.AppliedTimeScale);
		ImGui::Text("Paused State: %s", stats.Paused ? "true" : "false");

		ImGui::End();
	}

}
