#pragma once

#include <EngineCore.h>
#include <EngineRender.h>
#include "SceneSelectionContext.h"
#include "scene/ViewportCamera.h"

namespace Kita {

	class SceneViewportPanel
	{
	public:
		SceneViewportPanel(
			const Ref<SceneSelectionContext>& selectionContext,
			std::string windowName = "Viewport");

		SceneViewportPanel(const SceneViewportPanel&) = delete;
		SceneViewportPanel& operator=(const SceneViewportPanel&) = delete;

		~SceneViewportPanel();

		void Simulate(float daltaTime);
		void OnImGuiRender();
		void OnEvent(Event& event);
		void Render();
		VulkanRenderTarget* GetRenderTarget() const { return m_RenderTarget.get(); }
		ViewportCamera* GetViewportCamera() const { return m_ViewportCamera.get(); }
		void SetActive(bool isActive) { m_IsActive = isActive; }
		bool IsImageHovered() const { return m_IsImageHovered; }
		bool IsWindowFocused() const { return m_IsFocused; }
		bool IsOpen() const { return m_IsOpen; }

	private:
		void InitRenderResources();
		void ResizeRenderTargetIfNeeded();
		void RecreateViewportTexture();
		bool OnKeyPressed(KeyPressedEvent& event);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
		void TryPickObject();
		bool IsMouseInsideImageBounds() const;

	private:
		std::string m_WindowName = "Viewport";
		Unique<ViewportCamera> m_ViewportCamera = nullptr;

		Ref<SceneSelectionContext> m_SelectionContext = nullptr;

		Unique<VulkanRenderTarget> m_RenderTarget = nullptr;

		ImTextureID m_SceneTextureID = 0;
		glm::vec2 m_ViewportSize{ 1280.0f, 720.0f };
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
