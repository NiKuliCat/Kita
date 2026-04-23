#include "renderer_pch.h"
#include "EditorLayer.h"
#include "imgui.h"
#include "ImGuizmo.h"

#include <glm/glm.hpp>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>
#include <limits>
namespace Kita {

	EditorLayer::EditorLayer()
		:Layer("EditorLayer")
	{
		m_ViewportCamera = new ViewportCamera();
		m_CameraTransform = Transform();
		m_LightTransform = Transform();

		m_CameraTransform.SetPosition({ 0.0f,0.0f,5.0f });
		m_LightTransform.SetRotation({ 135.0,60.0f,0.0f });

		m_DirectLight = new Light();
	}
	void EditorLayer::OnCreate()
	{


		m_Scene = CreateRef<Scene>();

		{
			auto& obj = m_Scene->CreateObject("sphere");
			auto& meshrenderer = obj.AddComponent<MeshRenderer>();
			meshrenderer.LoadMeshs("assets/models/Sphere.fbx");
		}

		m_SceneHierarchyPanel = SceneHierarchyPanel(m_Scene);
		m_GizmoControlType = ImGuizmo::OPERATION::TRANSLATE;

		FrameBufferDescriptor disc;
		disc.AttachmentsDescription = { FrameBufferTexFormat::RGBA16F,FrameBufferTexFormat::DEPTH };
		disc.Width = 1280;
		disc.Height = 720;

		m_ViewportSize = { 1280,720 };

		m_FrameBuffer = FrameBuffer::Create(disc);
		m_SceneTexID = m_FrameBuffer->GetColorAttachment(0);
		RenderCommand::SetDepthTest(true);
		RenderCommand::SetBlend(true);
		RenderCommand::SetCullMode(CullMode::None);

		TextureDescriptor desc{};
		desc.EnableMipMaps = true;

		CubemapFacePaths faces = {
			"assets/textures/skybox/right.jpg",  // +X
			"assets/textures/skybox/left.jpg",   // -X
			"assets/textures/skybox/top.jpg",    // +Y
			"assets/textures/skybox/bottom.jpg", // -Y
			"assets/textures/skybox/front.jpg",  // +Z
			"assets/textures/skybox/back.jpg"    // -Z
		};

		m_Cubemap = Texture::CreateCubeMap(desc, faces);

	}

	void EditorLayer::OnUpdate(float daltaTime)
	{
		m_ViewportCamera->OnUpdate(daltaTime);
		DirectLightData light_data = DirectLightData();
		light_data.Color = glm::vec4(1.0, 1.0, 1.0, 1.0);
		light_data.Direction = glm::vec4(m_LightTransform.GetFrontDir(),1.0);

		auto desc = m_FrameBuffer->GetDescriptor();
		if (m_ViewportSize.x != desc.Width || m_ViewportSize.y != desc.Height)
		{
			m_FrameBuffer->ReSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_ViewportCamera->SetViewport(m_ViewportSize.x, m_ViewportSize.y);
		
		}
		auto vp = m_ViewportCamera->GetViewProjectionMatrix();


		m_FrameBuffer->Bind();
		Renderer::BeginScene(m_ViewportCamera->GetViewMatrix(),m_ViewportCamera->GetProjectionMatrix(),m_ViewportCamera->GetPosition(), light_data);

		RenderCommand::SetClearColor(glm::vec4(0.12, 0.12, 0.13, 1));
		RenderCommand::Clear();

		m_Scene->OnUpdate(daltaTime);


		Renderer::DrawSkyBox(m_Cubemap, 9);


		auto settings = EditorGridSettings{};
		settings.CellSize = 1.0f;
		Renderer::DrawEditorGrids(settings);





		Renderer::EndScene();
		m_FrameBuffer->UnBind();

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
				if (ImGui::MenuItem("New"))
				{

				}

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Window"))
			{
				if (ImGui::MenuItem("New"))
				{

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


			ImGui::EndMenuBar();
		}


		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		ImGuiWindowClass viewportWindowClass{};
		viewportWindowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton ;
		ImGui::SetNextWindowClass(&viewportWindowClass);
		ImGui::Begin("Viewport");
		uint32_t ScreenRT_ID = m_FrameBuffer->GetColorAttachment(0);
		{
			ImVec2 ViewportSize = ImGui::GetContentRegionAvail();
			m_ViewportSize = { ViewportSize.x,ViewportSize.y };
		}
		ImGui::Image(ScreenRT_ID, ImVec2{ m_ViewportSize.x,m_ViewportSize.y }, ImVec2(0, 1), ImVec2(1, 0));

		auto& selectedObj = m_SceneHierarchyPanel.GetSelectedObject();
		if (selectedObj)
		{
			ImGuizmo::SetDrawlist();
			float windowWidth = (float)ImGui::GetWindowWidth();
			float windowHeight = (float)ImGui::GetWindowHeight();
			ImGuizmo::SetRect(ImGui::GetWindowPos().x, ImGui::GetWindowPos().y, windowWidth, windowHeight);

			glm::mat4 viewMatrix = m_ViewportCamera->GetViewMatrix();
			glm::mat4 projectionMatrix = m_ViewportCamera->GetProjectionMatrix();

			auto& selectedObjTransform = selectedObj.GetComponent<Transform>();
			glm::mat4 modelMatrix = selectedObjTransform.GetTransformMatrix();

			bool enableSnapping = Input::IsKeyPressed(Key::LeftControl);
			float snappingValue = 0.1f;
			float snappingValues[3] = { snappingValue,snappingValue,snappingValue };


			ImGuizmo::SetGizmoSizeClipSpace(0.22f);
			auto& gizmoStyle = ImGuizmo::GetStyle();
			gizmoStyle.TranslationLineThickness = 2.0f; // 轴线粗细
			gizmoStyle.TranslationLineArrowSize = 5.0f; // 箭头大小
			gizmoStyle.CenterCircleSize = 4.5f;

			ImGuizmo::Manipulate(glm::value_ptr(viewMatrix), glm::value_ptr(projectionMatrix),
				ImGuizmo::OPERATION(m_GizmoControlType), ImGuizmo::LOCAL, glm::value_ptr(modelMatrix), nullptr, enableSnapping ? snappingValues : nullptr);

			if (ImGuizmo::IsUsing())
			{
				glm::vec3 translate, rotate, scale;
				Transform::DecomposeTransformMatrix(modelMatrix, translate, rotate, scale);
				//先统一转成弧度计算 
				glm::vec3 currentRotate = glm::radians(selectedObjTransform.GetRotation());
				glm::vec3 deltaRotate = rotate - currentRotate;
				currentRotate += deltaRotate;

				selectedObjTransform.SetPosition(translate);
				//最后转回角度
				selectedObjTransform.SetRotation(glm::degrees(currentRotate));
				selectedObjTransform.SetScale(scale);
			}

		}

		ImGui::End();
		ImGui::PopStyleVar();


		m_SceneHierarchyPanel.OnImGuiRender();



		ImGui::End();


	}

	void EditorLayer::OnEvent(Event& event)
	{
		EventDisPatcher dispatcher(event);
		dispatcher.Dispatcher<KeyPressedEvent>(BIND_EVENT_FUNC(EditorLayer::OnKeyPressed));
		m_ViewportCamera->OnEvent(event);
	}

	bool EditorLayer::OnKeyPressed(KeyPressedEvent& event)
	{
		if (event.IsRepeat())
			return false;

		switch (event.GetKeyCode())
		{
		case Key::W:
			m_GizmoControlType = ImGuizmo::OPERATION::TRANSLATE;
			break;
		case Key::E:
			m_GizmoControlType = ImGuizmo::OPERATION::ROTATE;
			break;
		case Key::R:
			m_GizmoControlType = ImGuizmo::OPERATION::SCALE;
			break;
		}
		return false;
	}

	

}

