#include "renderer_pch.h"
#include "SceneViewportPanel.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include <imgui_internal.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace Kita {

	SceneViewportPanel::SceneViewportPanel(const Ref<Scene>& scene, const Ref<SceneSelectionContext>& selectionContext, std::string windowName)
		: m_ViewportCamera(CreateUnique<ViewportCamera>()),
		m_SceneContext(scene),
		m_SelectionContext(selectionContext),
		m_WindowName(std::move(windowName))
	{
		InitFrameBuffer();

		m_LightTransform = Transform();
		m_LightTransform.SetRotation({ 135.0f, 60.0f, 0.0f });
		m_GizmoControlType = ImGuizmo::OPERATION::TRANSLATE;
	}

	void SceneViewportPanel::Simulate(float daltaTime)
	{
		if (!m_ViewportCamera)
			return;

		if (!m_IsActive)
			return;

		m_ViewportCamera->OnUpdate(daltaTime);
	}

	void SceneViewportPanel::OnImGuiRender()
	{
		if (!m_SelectionContext || !m_SceneContext)
			return;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		ImGuiWindowClass viewportWindowClass{};
		viewportWindowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
		ImGui::SetNextWindowClass(&viewportWindowClass);

		ImGui::Begin(m_WindowName.c_str());
		m_IsHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
		m_IsFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		m_ViewportSize = { viewportSize.x, viewportSize.y };
		auto viewportRegionMin = ImGui::GetWindowContentRegionMin();
		auto viewportRegionMax = ImGui::GetWindowContentRegionMax();
		auto viewportOffset = ImGui::GetWindowPos();
		m_ViewportBounds[0] = { viewportRegionMin.x + viewportOffset.x, viewportRegionMin.y + viewportOffset.y };
		m_ViewportBounds[1] = { viewportRegionMax.x + viewportOffset.x, viewportRegionMax.y + viewportOffset.y };

		ImGui::Image((ImTextureID)(uint64_t)m_SceneTexID, ImVec2{ m_ViewportSize.x, m_ViewportSize.y }, ImVec2(0, 1), ImVec2(1, 0));
		m_IsImageHovered = ImGui::IsItemHovered();

		auto& selectedObj = m_SelectionContext->GetSelectedObject();
		auto& selectedPoint = m_SelectionContext->GetSelectedPoint();

		if (selectedObj && selectedPoint.id != -1 && m_IsActive)
		{
			ImGuizmo::SetDrawlist();

			ImGuizmo::SetGizmoSizeClipSpace(0.22f);
			auto& gizmoStyle = ImGuizmo::GetStyle();
			gizmoStyle.TranslationLineThickness = 2.0f;
			gizmoStyle.TranslationLineArrowSize = 5.0f;
			gizmoStyle.CenterCircleSize = 0.0f;

			ImVec2 imageMin = ImGui::GetItemRectMin();
			ImVec2 imageMax = ImGui::GetItemRectMax();

			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(imageMin.x, imageMin.y, imageMax.x - imageMin.x, imageMax.y - imageMin.y);

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
		else if (selectedObj && m_IsActive)
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
			float snappingValues[3] = { snappingValue, snappingValue, snappingValue };

			ImGuizmo::SetGizmoSizeClipSpace(0.22f);
			auto& gizmoStyle = ImGuizmo::GetStyle();
			gizmoStyle.TranslationLineThickness = 2.0f;
			gizmoStyle.TranslationLineArrowSize = 5.0f;
			gizmoStyle.CenterCircleSize = 3.0f;

			ImGuizmo::Manipulate(
				glm::value_ptr(viewMatrix),
				glm::value_ptr(projectionMatrix),
				ImGuizmo::OPERATION(m_GizmoControlType),
				ImGuizmo::LOCAL,
				glm::value_ptr(modelMatrix),
				nullptr,
				enableSnapping ? snappingValues : nullptr);

			if (ImGuizmo::IsUsing())
			{
				glm::vec3 translate, rotate, scale;
				Transform::DecomposeTransformMatrix(modelMatrix, translate, rotate, scale);
				glm::vec3 currentRotate = glm::radians(selectedObjTransform.GetRotation());
				glm::vec3 deltaRotate = rotate - currentRotate;
				currentRotate += deltaRotate;

				selectedObjTransform.SetPosition(translate);
				selectedObjTransform.SetRotation(glm::degrees(currentRotate));
				selectedObjTransform.SetScale(scale);
			}
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void SceneViewportPanel::OnEvent(Event& event)
	{
		if (!m_ViewportCamera)
			return;

		bool allowInput = false;
		if (event.IsInCategory(EventCategory::EventMouse))
			allowInput = m_IsActive && m_IsImageHovered;
		else if (event.IsInCategory(EventCategory::EventKeyboard))
			allowInput = m_IsActive && (m_IsFocused || m_IsImageHovered);
		else
			allowInput = m_IsActive && (m_IsImageHovered || m_IsFocused);

		if (!allowInput)
			return;

		EventDisPatcher dispatcher(event);
		dispatcher.Dispatcher<KeyPressedEvent>(BIND_EVENT_FUNC(SceneViewportPanel::OnKeyPressed));
		dispatcher.Dispatcher<MouseButtonPressedEvent>(BIND_EVENT_FUNC(SceneViewportPanel::OnMouseButtonPressed));
		m_ViewportCamera->OnEvent(event);
	}

	void SceneViewportPanel::Render()
	{
		if (!m_SelectionContext || !m_SceneContext || !m_ViewportCamera)
			return;

		if (!m_SceneMSAAFrameBuffer || !m_SceneResolveFrameBuffer || !m_PickingFrameBuffer)
		{
			InitFrameBuffer();
			if (!m_SceneMSAAFrameBuffer || !m_SceneResolveFrameBuffer || !m_PickingFrameBuffer)
				return;
		}

		if (m_ViewportSize.x <= 0.0f || m_ViewportSize.y <= 0.0f)
			return;

		auto resolveDesc = m_SceneResolveFrameBuffer->GetDescriptor();
		if (m_ViewportSize.x != resolveDesc.Width || m_ViewportSize.y != resolveDesc.Height)
		{
			const uint32_t width = (uint32_t)m_ViewportSize.x;
			const uint32_t height = (uint32_t)m_ViewportSize.y;

			m_SceneMSAAFrameBuffer->ReSize(width, height);
			m_SceneResolveFrameBuffer->ReSize(width, height);
			m_PickingFrameBuffer->ReSize(width, height);

			m_ViewportCamera->SetViewport(m_ViewportSize.x, m_ViewportSize.y);
			m_SceneTexID = m_SceneResolveFrameBuffer->GetColorAttachment(0);
		}

		DirectLightData lightData{};
		lightData.Color = glm::vec4(1.0f);
		lightData.Direction = glm::vec4(m_LightTransform.GetFrontDir(), 1.0f);

		m_PickingFrameBuffer->Bind();
		Renderer::BeginScene(
			m_ViewportCamera->GetViewMatrix(),
			m_ViewportCamera->GetProjectionMatrix(),
			m_ViewportCamera->GetPosition(),
			lightData,
			{ m_PickingFrameBuffer->GetSize() });

		RenderCommand::SetClearColor(glm::vec4(0.12f, 0.12f, 0.13f, 1.0f));
		RenderCommand::Clear();
		m_PickingFrameBuffer->ClearIDBuffer(-1, 1);
		m_PickingFrameBuffer->ClearIDBuffer(-1, 2);

		m_SceneContext->RenderSceneEditor();

		Renderer::EndScene();
		m_PickingFrameBuffer->UnBind();

		m_SceneMSAAFrameBuffer->Bind();
		Renderer::BeginScene(
			m_ViewportCamera->GetViewMatrix(),
			m_ViewportCamera->GetProjectionMatrix(),
			m_ViewportCamera->GetPosition(),
			lightData,
			{ m_SceneMSAAFrameBuffer->GetSize() });

		RenderCommand::SetClearColor(glm::vec4(0.12f, 0.12f, 0.13f, 1.0f));
		RenderCommand::Clear();

		m_SceneContext->RenderSceneEditor();

		Renderer::EndScene();
		m_SceneMSAAFrameBuffer->UnBind();

		m_SceneMSAAFrameBuffer->BlitColorTo(m_SceneResolveFrameBuffer, 0, 0);
	}

	void SceneViewportPanel::InitFrameBuffer()
	{
		m_ViewportSize = { 1280, 720 };
		FrameBufferDescriptor sceneMSAADesc;
		sceneMSAADesc.AttachmentsDescription = {
			FrameBufferTexFormat::RGBA16F,
			FrameBufferTexFormat::DEPTH
		};
		sceneMSAADesc.Width = 1280;
		sceneMSAADesc.Height = 720;
		sceneMSAADesc.Samples = 4;
		m_SceneMSAAFrameBuffer = FrameBuffer::Create(sceneMSAADesc);

		FrameBufferDescriptor sceneResolveDesc;
		sceneResolveDesc.AttachmentsDescription = {
			FrameBufferTexFormat::RGBA16F,
			FrameBufferTexFormat::DEPTH
		};
		sceneResolveDesc.Width = 1280;
		sceneResolveDesc.Height = 720;
		sceneResolveDesc.Samples = 1;
		m_SceneResolveFrameBuffer = FrameBuffer::Create(sceneResolveDesc);

		FrameBufferDescriptor pickingDesc;
		pickingDesc.AttachmentsDescription = {
			FrameBufferTexFormat::RGBA16F,
			FrameBufferTexFormat::RED_INTEGER,
			FrameBufferTexFormat::RED_INTEGER,
			FrameBufferTexFormat::DEPTH
		};
		pickingDesc.Width = 1280;
		pickingDesc.Height = 720;
		pickingDesc.Samples = 1;
		m_PickingFrameBuffer = FrameBuffer::Create(pickingDesc);

		m_SceneTexID = m_SceneResolveFrameBuffer->GetColorAttachment(0);
	}

	bool SceneViewportPanel::OnKeyPressed(KeyPressedEvent& event)
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

	bool SceneViewportPanel::OnMouseButtonPressed(MouseButtonPressedEvent& event)
	{
		if (m_IsActive && event.GetMouseButton() == Mouse::Button0 && IsMouseInsideImageBounds())
		{
			TryPickObject();
		}
		return false;
	}

	bool SceneViewportPanel::IsMouseInsideImageBounds() const
	{
		auto [mx, my] = ImGui::GetMousePos();
		return mx >= m_ViewportBounds[0].x && mx < m_ViewportBounds[1].x
			&& my >= m_ViewportBounds[0].y && my < m_ViewportBounds[1].y;
	}

	void SceneViewportPanel::TryPickObject()
	{
		if (!m_SelectionContext || !m_SceneContext)
			return;

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

		m_PickingFrameBuffer->Bind();

		int mouseX = (int)mx;
		int mouseY = (int)(size.y - my);
		mouseX = std::clamp(mouseX, 0, (int)size.x - 1);
		mouseY = std::clamp(mouseY, 0, (int)size.y - 1);

		int pixelId = m_PickingFrameBuffer->GetIDBufferValue(mouseX, mouseY, 1);
		int pixelIndex = m_PickingFrameBuffer->GetIDBufferValue(mouseX, mouseY, 2);

		if (pixelId == -1)
		{
			pixelIndex = -1;
		}

		if (ImGuizmo::IsOver() && pixelIndex == -1)
		{
			m_PickingFrameBuffer->UnBind();
			return;
		}

		if (pixelIndex != -1 && pixelId != -1)
		{
			Object selectedObject = Object{ (entt::entity)pixelId, m_SceneContext.get() };
			if (selectedObject.HasComponent<LineRenderer>())
			{
				auto& lineRenderer = selectedObject.GetComponent<LineRenderer>();
				m_SelectionContext->ClearSelectedPoint();
				m_SelectionContext->SetSelection(selectedObject);
				m_SelectionContext->SetSelectedPoint(lineRenderer.GetControlPointByIndex(pixelIndex));
				lineRenderer.SetSelectedControlPoint(pixelIndex);

				const glm::vec4 highlightColor = lineRenderer.IsAnchorControlPoint(pixelIndex)
					? glm::vec4(0.95f, 0.90f, 0.22f, 1.0f)
					: glm::vec4(0.55f, 0.85f, 1.0f, 1.0f);
				lineRenderer.SetControlPointColorByIndex(highlightColor, pixelIndex);
			}
		}
		else if (pixelId != -1)
		{
			Object selectedObject = Object{ (entt::entity)pixelId, m_SceneContext.get() };
			m_SelectionContext->ClearSelectedPoint();
			m_SelectionContext->SetSelection(selectedObject);
			m_SelectionContext->SetSelectedPoint({});
		}
		else
		{
			m_SelectionContext->ClearSelection();
		}

		KITA_CLENT_INFO("pixel in viewport,id :{0},index :{1}", pixelId, pixelIndex);
		m_PickingFrameBuffer->UnBind();
	}
}
