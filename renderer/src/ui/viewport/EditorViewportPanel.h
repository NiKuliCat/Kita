#pragma once
#include <EngineCore.h>
#include "scene/ViewportCamera.h"
#include "imgui.h"
namespace Kita {

	struct ViewportPickRequest
	{
		uint32_t PixelX = 0;
		uint32_t PixelY = 0;
	};

	struct ViewportOverlaySettings
	{
		bool ShowGrid = true;
		float FlightSpeedScale = 1.0f;
		float RotationSpeed = 0.9f;
		float ZoomSpeedScale = 1.0f;
		float MinorCellSize = 1.0f;
		float MajorCellSize = 10.0f;
		float MinorLineWidth = 1.15f;
		float MajorLineWidth = 1.85f;
		float FadeNear = 1.0f;
		float FadeFar = 120.0f;
		float FadeAngleStart = 0.02f;
		float DepthBias = 0.005f;
	};

	class EditorViewportPanel
	{
	public:
		EditorViewportPanel(std::string windowName = "Viewport");

		EditorViewportPanel(const EditorViewportPanel&) = delete;
		EditorViewportPanel& operator=(const EditorViewportPanel&) = delete;

		~EditorViewportPanel() = default;

		void OnUpdata(Timestep ts);
		void OnImGuiRender();
		void OnEvent(Event& event);

		void SetDisplayTexture(ImTextureID textureID) { m_DisplayTextureID = textureID; }
		ImTextureID GetDisplayTexture() const { return m_DisplayTextureID; }
		void ClearDisplayTexture() { m_DisplayTextureID = 0; }
		ViewportOverlaySettings& GetOverlaySettings() { return m_OverlaySettings; }
		const ViewportOverlaySettings& GetOverlaySettings() const { return m_OverlaySettings; }

		ViewportCamera* GetViewportCamera() const { return m_ViewportCamera.get(); }
		const glm::vec2& GetDesiredViewportSize() const { return m_ViewportSize; }

		bool HasPendingPickRequest() const { return m_HasPendingPickRequest; }
		ViewportPickRequest ConsumePickRequest();

		void SetActive(bool isActive) { m_IsActive = isActive; }
		bool IsImageHovered() const { return m_IsImageHovered; }
		bool IsWindowFocused() const { return m_IsFocused; }
		bool IsOpen() const { return m_IsOpen; }

	private:
		bool OnKeyPressed(KeyPressedEvent& event);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
		void RequestPickAtMousePosition();
		bool IsMouseInsideImageBounds() const;
		void DrawViewportOverlay();

	private:
		std::string m_WindowName = "Viewport";
		Unique<ViewportCamera> m_ViewportCamera = nullptr;

		ImTextureID m_DisplayTextureID = 0;
		glm::vec2 m_ViewportSize{ 1280.0f, 720.0f };
		glm::vec2 m_ViewportBounds[2]{};

		int32_t m_GizmoControlType = 1;
		ViewportPickRequest m_PendingPickRequest{};
		bool m_HasPendingPickRequest = false;

		bool m_IsHovered = false;
		bool m_IsFocused = false;
		bool m_IsImageHovered = false;
		bool m_IsOverlayHovered = false;
		bool m_IsActive = false;
		bool m_IsOpen = true;
		bool m_UseInitialPlacement = true;
		bool m_RequestWindowFocus = true;
		bool m_ShowOverlaySettingsMenu = false;
		double m_OverlaySettingsKeepAliveUntil = 0.0;
		ViewportOverlaySettings m_OverlaySettings{};
	};

}
