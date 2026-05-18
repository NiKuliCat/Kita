#pragma once
#include <EngineCore.h>
#include "imgui.h"

#include <functional>

namespace Kita {

	struct ViewportPickRequest
	{
		uint32_t PixelX = 0;
		uint32_t PixelY = 0;
	};

	struct ViewportPanelFrameState
	{
		glm::vec2 ViewportSize{};
		glm::vec2 BoundsMin{};
		glm::vec2 BoundsMax{};
		bool IsOpen = true;
		bool IsFocused = false;
		bool IsWindowHovered = false;
		bool IsImageHovered = false;
		bool IsOverlayHovered = false;
		bool WantsPick = false;
		ViewportPickRequest PickRequest{};
	};

	struct ViewportOverlaySettings
	{
		bool ShowGrid = true;
		float FlightSpeedScale = 8.0f;
		float RotationSpeed = 0.9f;
		float ZoomSpeedScale = 1.0f;
		float MinorCellSize = 1.0f;
		float MajorCellSize = 10.0f;
		float MinorLineWidth = 1.15f;
		float MajorLineWidth = 1.85f;
		float FadeNear = 1.0f;
		float FadeFar = 190.0f;
		float FadeAngleStart = 0.02f;
		float DepthBias = 0.005f;
	};

	class EditorViewportPanel
	{
	public:
		using ViewportContentCallback = std::function<void(const ViewportPanelFrameState&)>;

		EditorViewportPanel(std::string windowName = "Viewport");

		EditorViewportPanel(const EditorViewportPanel&) = delete;
		EditorViewportPanel& operator=(const EditorViewportPanel&) = delete;

		~EditorViewportPanel() = default;

		void OnUpdata(Timestep ts);
		const ViewportPanelFrameState& OnImGuiRender(const ViewportContentCallback& viewportContentCallback = {});
		void OnEvent(Event& event);
		bool ShouldHandleEvent(Event& event) const;

		void SetDisplayTexture(ImTextureID textureID) { m_DisplayTextureID = textureID; }
		ImTextureID GetDisplayTexture() const { return m_DisplayTextureID; }
		void ClearDisplayTexture() { m_DisplayTextureID = 0; }
		void SetGizmoInteractionState(bool isOver, bool isUsing)
		{
			m_GizmoOver = isOver;
			m_GizmoUsing = isUsing;
		}
		ViewportOverlaySettings& GetOverlaySettings() { return m_OverlaySettings; }
		const ViewportOverlaySettings& GetOverlaySettings() const { return m_OverlaySettings; }
		const ViewportPanelFrameState& GetFrameState() const { return m_FrameState; }

		ViewportPickRequest ConsumePickRequest();

		void SetActive(bool isActive) { m_IsActive = isActive; }
		bool IsImageHovered() const { return m_FrameState.IsImageHovered; }
		bool IsWindowFocused() const { return m_FrameState.IsFocused; }
		bool IsOpen() const { return m_FrameState.IsOpen; }

	private:
		bool OnKeyPressed(KeyPressedEvent& event);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
		void RequestPickAtMousePosition();
		bool IsMouseInsideImageBounds() const;
		void DrawViewportOverlay();

	private:
		std::string m_WindowName = "Viewport";

		ImTextureID m_DisplayTextureID = 0;
		ViewportPanelFrameState m_FrameState{
			glm::vec2(1280.0f, 720.0f)
		};
		bool m_IsActive = false;
		bool m_UseInitialPlacement = true;
		bool m_RequestWindowFocus = true;
		bool m_ShowOverlaySettingsMenu = false;
		double m_OverlaySettingsKeepAliveUntil = 0.0;
		bool m_GizmoOver = false;
		bool m_GizmoUsing = false;
		ViewportOverlaySettings m_OverlaySettings{};
	};

}
