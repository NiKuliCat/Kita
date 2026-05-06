#pragma once
#include "Engine.h"
#include "scene/ViewportCamera.h"
#include "SceneSelectionContext.h"

namespace Kita {

	class SceneViewportPanel
	{
	public:
		SceneViewportPanel(
			const Ref<Scene>& scene,
			const Ref<SceneSelectionContext>& selectionContext,
			std::string windowName = "Viewport");

		SceneViewportPanel(const SceneViewportPanel&) = delete;
		SceneViewportPanel& operator=(const SceneViewportPanel&) = delete;


		void Simulate(float daltaTime);
		void OnImGuiRender();
		void OnEvent(Event& event);
		void Render();
		void SetActive(bool isActive) { m_IsActive = isActive; }
		bool IsImageHovered() const { return m_IsImageHovered; }
		bool IsWindowFocused() const { return m_IsFocused; }
		bool IsOpen() const { return m_IsOpen; }

	private:
		void InitFrameBuffer();
		bool OnKeyPressed(KeyPressedEvent& event);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
		void TryPickObject();
		bool IsMouseInsideImageBounds() const;

	private:
		std::string m_WindowName = "Viewport";
		Unique<ViewportCamera> m_ViewportCamera = nullptr;

		Ref<FrameBuffer> m_SceneMSAAFrameBuffer = nullptr;
		Ref<FrameBuffer> m_SceneResolveFrameBuffer = nullptr;
		Ref<FrameBuffer> m_PickingFrameBuffer = nullptr;

		Ref<Scene> m_SceneContext = nullptr;
		Ref<SceneSelectionContext> m_SelectionContext = nullptr;

		uint32_t m_SceneTexID = 0;
		glm::vec2 m_ViewportSize{};
		glm::vec2 m_ViewportBounds[2]{};
		int32_t m_GizmoControlType = 1;
		bool m_IsHovered = false;
		bool m_IsFocused = false;
		bool m_IsImageHovered = false;
		bool m_IsActive = false;
		bool m_IsOpen = true;
		bool m_UseInitialPlacement = true;
		bool m_RequestWindowFocus = true;
	};

}
