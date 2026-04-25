#include "renderer_pch.h"
#include "EditorLayer.h"
#include "imgui.h"
#include "ImGuizmo.h"

#include <glm/glm.hpp>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>

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


		m_Scene = CreateRef<Scene>( "example scene");

		{
			auto obj = m_Scene->CreateObject("sphere");
			auto& meshrenderer = obj.AddComponent<MeshRenderer>();
			meshrenderer.LoadMeshs("assets/models/Sphere.fbx");

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

		CubemapFacePaths faces = {
			"assets/textures/skybox/right.jpg",  // +X
			"assets/textures/skybox/left.jpg",   // -X
			"assets/textures/skybox/top.jpg",    // +Y
			"assets/textures/skybox/bottom.jpg", // -Y
			"assets/textures/skybox/front.jpg",  // +Z
			"assets/textures/skybox/back.jpg"    // -Z
		};

		m_Scene->LoadSkyCubemap(faces);



		m_SceneHierarchyPanel = SceneHierarchyPanel(m_Scene);
		m_GizmoControlType = ImGuizmo::OPERATION::TRANSLATE;

		FrameBufferDescriptor disc;
		disc.AttachmentsDescription = { FrameBufferTexFormat::RGBA16F, FrameBufferTexFormat::RED_INTEGER, FrameBufferTexFormat::RED_INTEGER,FrameBufferTexFormat::DEPTH };
		disc.Width = 1280;
		disc.Height = 720;

		m_ViewportSize = { 1280,720 };

		m_FrameBuffer = FrameBuffer::Create(disc);
		m_SceneTexID = m_FrameBuffer->GetColorAttachment(0);
		RenderCommand::SetDepthTest(true);
		RenderCommand::SetBlend(true);
		RenderCommand::SetCullMode(CullMode::None);





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
		Renderer::BeginScene(m_ViewportCamera->GetViewMatrix(),m_ViewportCamera->GetProjectionMatrix(),m_ViewportCamera->GetPosition(), light_data, {m_FrameBuffer->GetSize()});

		RenderCommand::SetClearColor(glm::vec4(0.12, 0.12, 0.13, 1));
		RenderCommand::Clear();
		m_FrameBuffer->ClearIDBuffer(-1,1);
		m_FrameBuffer->ClearIDBuffer(-1,2);

		m_Scene->OnUpdate(daltaTime);



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

		auto viewportReginMin = ImGui::GetWindowContentRegionMin();
		auto viewportReginMax = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();
		m_ViewportBounds[0] = { viewportReginMin.x + viewportOffset.x,viewportReginMin.y + viewportOffset.y };
		m_ViewportBounds[1] = { viewportReginMax.x + viewportOffset.x,viewportReginMax.y + viewportOffset.y };

		ImGui::Image(ScreenRT_ID, ImVec2{ m_ViewportSize.x,m_ViewportSize.y }, ImVec2(0, 1), ImVec2(1, 0));

		auto& selectedObj = m_SceneHierarchyPanel.GetSelectedObject();
		auto& selectedPoint = m_SceneHierarchyPanel.GetSelectedPoint();
		
		if (selectedObj && selectedPoint.id != -1)
		{
			ImGuizmo::SetDrawlist();

			ImGuizmo::SetGizmoSizeClipSpace(0.22f);
			auto& gizmoStyle = ImGuizmo::GetStyle();
			gizmoStyle.TranslationLineThickness = 2.0f; // 轴线粗细
			gizmoStyle.TranslationLineArrowSize = 5.0f; // 箭头大小
			gizmoStyle.CenterCircleSize = 0.0f;

			ImVec2 imageMin = ImGui::GetItemRectMin();
			ImVec2 imageMax = ImGui::GetItemRectMax();

			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(imageMin.x,imageMin.y,imageMax.x - imageMin.x,imageMax.y - imageMin.y);




			glm::mat4 viewMatrix = m_ViewportCamera->GetViewMatrix();
			glm::mat4 projectionMatrix = m_ViewportCamera->GetProjectionMatrix();

			auto& transform = selectedObj.GetComponent<Transform>();
			glm::mat4 ownerModel = transform.GetTransformMatrix();

			glm::vec3 worldPointPos = glm::vec3(ownerModel * glm::vec4(selectedPoint.position, 1.0f));
			glm::mat4 pointMatrix = glm::translate(glm::mat4(1.0f), worldPointPos);
			bool enableSnapping = Input::IsKeyPressed(Key::LeftControl);
			float snappingValue = 0.1f;
			float snappingValues[3] = { snappingValue, snappingValue, snappingValue };

			ImGuizmo::SetGizmoSizeClipSpace(0.18f);

			ImGuizmo::Manipulate(
				glm::value_ptr(viewMatrix),
				glm::value_ptr(projectionMatrix),
				ImGuizmo::TRANSLATE,
				ImGuizmo::WORLD,
				glm::value_ptr(pointMatrix),
				nullptr,
				enableSnapping ? snappingValues : nullptr
			);

			if (ImGuizmo::IsUsing())
			{
				glm::vec3 newWorldPos, rotate, scale;
				Transform::DecomposeTransformMatrix(pointMatrix, newWorldPos, rotate, scale);

				glm::vec3 newLocalPos = glm::vec3(glm::inverse(ownerModel) * glm::vec4(newWorldPos, 1.0f));

				if (selectedObj.HasComponent<LineRenderer>())
				{
					auto& lineRenderer = selectedObj.GetComponent<LineRenderer>();

					lineRenderer.MoveControlPoint(selectedPoint.id, newLocalPos);

					selectedPoint = lineRenderer.GetControlPointByIndex(selectedPoint.id);
				}
			}
		}
		else if (selectedObj)
		{
			ImGuizmo::SetDrawlist();
			ImVec2 imageMin = ImGui::GetItemRectMin();
			ImVec2 imageMax = ImGui::GetItemRectMax();

			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(imageMin.x, imageMin.y, imageMax.x - imageMin.x, imageMax.y - imageMin.y);


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
			gizmoStyle.CenterCircleSize = 3.0f;

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
		dispatcher.Dispatcher<MouseButtonPressedEvent>(BIND_EVENT_FUNC(EditorLayer::OnMouseButtonPressed));
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

	bool EditorLayer::OnMouseButtonPressed(MouseButtonPressedEvent& event)
	{
		if (event.GetMouseButton() == Mouse::Button0)
		{
			TryPickObject();
		}
		return false;
	}

	void EditorLayer::TryPickObject()
	{
		if (Input::IsKeyPressed(Key::LeftAlt))
			return;

		if (ImGuizmo::IsUsing())
			return;

		auto [mx, my] = ImGui::GetMousePos();
		mx -= m_ViewportBounds[0].x;
		my -= m_ViewportBounds[0].y;
		glm::vec2 size = m_ViewportBounds[1] - m_ViewportBounds[0];

		if (mx < 0.0f || my < 0.0f || mx >= size.x || my >= size.y)
			return;

		m_FrameBuffer->Bind();

		int mouseX = (int)mx;
		int mouseY = (int)(size.y - my);
		mouseX = std::clamp(mouseX, 0, (int)size.x - 1);
		mouseY = std::clamp(mouseY, 0, (int)size.y - 1);

		int pixel_id = m_FrameBuffer->GetIDBufferValue(mouseX, mouseY, 1);
		int pixel_index = m_FrameBuffer->GetIDBufferValue(mouseX, mouseY, 2);

		if (pixel_id == -1)
		{
			pixel_index = -1;
		}

		// 点击到 ImGuizmo 本体时，不要触发对象重新拾取或清空选择。
		// 但如果当前像素明确是控制点，则仍然允许控制点拾取。
		if (ImGuizmo::IsOver() && pixel_index == -1)
		{
			m_FrameBuffer->UnBind();
			return;
		}

		//目前如果index不等于-1 则说明该对象有lineRenderer组件,硬编
		if (pixel_index != -1 && pixel_id != -1)
		{
			Object selectedObject = Object{ (entt::entity)pixel_id,m_Scene.get() };
			if (selectedObject.HasComponent<LineRenderer>())
			{
				auto& lineRenderer = selectedObject.GetComponent<LineRenderer>();
				m_SceneHierarchyPanel.ClearSelectedPoint();
				m_SceneHierarchyPanel.SetSelectedObject(selectedObject);
				m_SceneHierarchyPanel.SetSelectedPoint(lineRenderer.GetControlPointByIndex(pixel_index));
				lineRenderer.SetSelectedControlPoint(pixel_index);

				const glm::vec4 highlightColor = lineRenderer.IsAnchorControlPoint(pixel_index)
					? glm::vec4(0.95f, 0.90f, 0.22f, 1.0f)
					: glm::vec4(0.55f, 0.85f, 1.0f, 1.0f);
				lineRenderer.SetControlPointColorByIndex(highlightColor, pixel_index);
			}
			
		}

		else if (pixel_id != -1)
		{
			Object selectedObject = Object{ (entt::entity)pixel_id, m_Scene.get() };

			m_SceneHierarchyPanel.ClearSelectedPoint();
			m_SceneHierarchyPanel.SetSelectedObject(selectedObject);
			m_SceneHierarchyPanel.SetSelectedPoint({});
		}
		else
		{
			m_SceneHierarchyPanel.ClearSelection();
		}
		KITA_CLENT_INFO("pixel in viewport,id :{0},index :{1}", pixel_id, pixel_index);

		m_FrameBuffer->UnBind();
	}

	

}

