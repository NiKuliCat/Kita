#include "renderer_pch.h"
#include "EditorViewportPanel.h"

#include "imgui.h"
#include <imgui_internal.h>

#include <algorithm>

namespace  Kita {

	namespace
	{
		constexpr float kOverlayPadding = 10.0f;
		constexpr float kOverlayButtonSize = 28.0f;

		void DrawMissingTexturePlaceholder(const ImVec2& min, const ImVec2& max)
		{
			ImDrawList* drawList = ImGui::GetWindowDrawList();

			const ImU32 bgColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.08f, 0.08f, 0.09f, 1.0f));
			const ImU32 borderColor = ImGui::ColorConvertFloat4ToU32(ImVec4(0.18f, 0.18f, 0.20f, 1.0f));
			drawList->AddRectFilled(min, max, bgColor);
			drawList->AddRect(min, max, borderColor);

			const char* text = "Viewport texture unavailable";
			const ImVec2 textSize = ImGui::CalcTextSize(text);
			const ImVec2 center(
				min.x + (max.x - min.x - textSize.x) * 0.5f,
				min.y + (max.y - min.y - textSize.y) * 0.5f);
			drawList->AddText(center, ImGui::GetColorU32(ImGuiCol_TextDisabled), text);
		}
	}


	EditorViewportPanel::EditorViewportPanel(std::string windowName)
		: m_WindowName(std::move(windowName))
	{
	}

	void EditorViewportPanel::OnUpdata(Timestep ts)
	{
		(void)ts;
	}

	const ViewportPanelFrameState& EditorViewportPanel::OnImGuiRender(const ViewportContentCallback& viewportContentCallback)
	{
		m_FrameState.WantsPick = false;
		m_FrameState.PickRequest = {};
		m_FrameState.IsOverlayHovered = false;
		m_FrameState.IsWindowHovered = false;
		m_FrameState.IsFocused = false;
		m_FrameState.IsImageHovered = false;

		if (!m_FrameState.IsOpen)
			return m_FrameState;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGuiWindowClass viewportWindowClass{};
		viewportWindowClass.DockNodeFlagsOverrideSet = ImGuiDockNodeFlags_NoWindowMenuButton;
		ImGui::SetNextWindowClass(&viewportWindowClass);

		if (m_UseInitialPlacement)
		{
			const ImGuiViewport* mainViewport = ImGui::GetMainViewport();
			const ImGuiID windowId = ImHashStr(m_WindowName.c_str());
			const float offsetX = 48.0f * static_cast<float>(windowId % 4u);
			const float offsetY = 40.0f * static_cast<float>((windowId / 4u) % 4u);

			ImGui::SetNextWindowDockID(0, ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowPos(
				ImVec2(mainViewport->WorkPos.x + 72.0f + offsetX, mainViewport->WorkPos.y + 72.0f + offsetY),
				ImGuiCond_FirstUseEver);
			ImGui::SetNextWindowSize(ImVec2(720.0f, 460.0f), ImGuiCond_FirstUseEver);
		}

		if (m_RequestWindowFocus)
			ImGui::SetNextWindowFocus();

		ImGui::Begin(m_WindowName.c_str());
		m_UseInitialPlacement = false;
		m_RequestWindowFocus = false;

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		const ImRect titleBarRect = window->TitleBarRect();
		if (ImGui::IsMouseHoveringRect(titleBarRect.Min, titleBarRect.Max, false) &&
			ImGui::IsMouseClicked(ImGuiMouseButton_Right))
		{
			ImGui::OpenPopup("ViewportTitleContextMenu");
		}

		if (ImGui::BeginPopup("ViewportTitleContextMenu"))
		{
			if (ImGui::MenuItem("Close"))
			{
				m_FrameState.IsOpen = false;
				m_FrameState.IsWindowHovered = false;
				m_FrameState.IsFocused = false;
				m_FrameState.IsImageHovered = false;
			}
			ImGui::EndPopup();
		}

		if (!m_FrameState.IsOpen)
		{
			ImGui::End();
			ImGui::PopStyleVar();
			return m_FrameState;
		}

		m_FrameState.IsWindowHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
		m_FrameState.IsFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		m_FrameState.IsOverlayHovered = false;

		const ImVec2 contentSize = ImGui::GetContentRegionAvail();
		if (contentSize.x > 1.0f && contentSize.y > 1.0f)
		{
			m_FrameState.ViewportSize = { contentSize.x, contentSize.y };
		}

		const ImVec2 imageSize(
			std::max(1.0f, m_FrameState.ViewportSize.x),
			std::max(1.0f, m_FrameState.ViewportSize.y));

		if (m_DisplayTextureID)
		{
			ImGui::Image(
				m_DisplayTextureID,
				imageSize,
				ImVec2(0.0f, 1.0f),
				ImVec2(1.0f, 0.0f));
		}
		else
		{
			ImGui::Dummy(imageSize);
			DrawMissingTexturePlaceholder(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
		}

		const ImVec2 itemMin = ImGui::GetItemRectMin();
		const ImVec2 itemMax = ImGui::GetItemRectMax();
		m_FrameState.BoundsMin = { itemMin.x, itemMin.y };
		m_FrameState.BoundsMax = { itemMax.x, itemMax.y };
		m_FrameState.IsImageHovered = m_FrameState.IsWindowHovered && IsMouseInsideImageBounds();
		if (viewportContentCallback)
			viewportContentCallback(m_FrameState);
		DrawViewportOverlay();
		if (m_FrameState.IsOverlayHovered)
			m_FrameState.IsImageHovered = false;

		const bool leftClicked = ImGui::IsMouseClicked(ImGuiMouseButton_Left);
		if (leftClicked && m_IsActive && !m_FrameState.IsImageHovered)
		{
			KITA_CORE_INFO(
				"Viewport pick suppressed: leftClicked=1 active=1 imageHovered=0 windowHovered={} overlayHovered={} gizmoOver={} gizmoUsing={}",
				m_FrameState.IsWindowHovered,
				m_FrameState.IsOverlayHovered,
				m_GizmoOver,
				m_GizmoUsing);
		}
		else if (leftClicked && m_IsActive && m_FrameState.IsImageHovered && m_GizmoUsing)
		{
			KITA_CORE_INFO(
				"Viewport pick suppressed: leftClicked=1 active=1 imageHovered=1 gizmoUsing=1 gizmoOver={}",
				m_GizmoOver);
		}

		if (m_IsActive &&
			m_FrameState.IsImageHovered &&
			leftClicked &&
			!m_GizmoUsing)
		{
			RequestPickAtMousePosition();
		}

		ImGui::End();
		ImGui::PopStyleVar();
		return m_FrameState;
	}

	void EditorViewportPanel::OnEvent(Event& event)
	{
		(void)event;
	}

	bool EditorViewportPanel::ShouldHandleEvent(Event& event) const
	{
		bool allowInput = false;
		if (event.IsInCategory(EventCategory::EventMouse))
			allowInput = m_IsActive && m_FrameState.IsImageHovered;
		else if (event.IsInCategory(EventCategory::EventKeyboard))
			allowInput = m_IsActive && (m_FrameState.IsFocused || m_FrameState.IsImageHovered);
		else
			allowInput = m_IsActive && (m_FrameState.IsImageHovered || m_FrameState.IsFocused);

		return allowInput;
	}

	ViewportPickRequest EditorViewportPanel::ConsumePickRequest()
	{
		ViewportPickRequest request = m_FrameState.PickRequest;
		m_FrameState.PickRequest = {};
		m_FrameState.WantsPick = false;
		return request;
	}

	bool EditorViewportPanel::OnKeyPressed(KeyPressedEvent& event)
	{
		(void)event;
		return false;
	}

	bool EditorViewportPanel::OnMouseButtonPressed(MouseButtonPressedEvent& event)
	{
		(void)event;
		return false;
	}

	void EditorViewportPanel::DrawViewportOverlay()
	{
		ImGui::SetCursorScreenPos(ImVec2(
			m_FrameState.BoundsMin.x + kOverlayPadding,
			m_FrameState.BoundsMin.y + kOverlayPadding));

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.11f, 0.13f, 0.92f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.20f, 0.24f, 0.96f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.24f, 0.28f, 0.98f));

		if (ImGui::Button(m_OverlaySettings.ShowGrid ? "[#]" : "[ ]", ImVec2(kOverlayButtonSize, kOverlayButtonSize)))
			m_OverlaySettings.ShowGrid = !m_OverlaySettings.ShowGrid;
		m_FrameState.IsOverlayHovered |= ImGui::IsItemHovered();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Toggle Grid");

		ImGui::SameLine(0.0f, 6.0f);
		ImGui::Button("[=]", ImVec2(kOverlayButtonSize, kOverlayButtonSize));
		const bool settingsHovered = ImGui::IsItemHovered();
		m_FrameState.IsOverlayHovered |= settingsHovered;
		if (settingsHovered)
			ImGui::SetTooltip("Viewport Settings");

		const ImVec2 buttonMin = ImGui::GetItemRectMin();
		const ImVec2 buttonMax = ImGui::GetItemRectMax();
		const ImVec2 menuPos(buttonMin.x, buttonMax.y + 6.0f);
		const double currentTime = ImGui::GetTime();

		if (settingsHovered)
		{
			m_ShowOverlaySettingsMenu = true;
			m_OverlaySettingsKeepAliveUntil = currentTime + 0.20;
		}

		ImGui::SetNextWindowPos(menuPos, ImGuiCond_Always);
		ImGui::SetNextWindowBgAlpha(0.96f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10.0f, 10.0f));
		ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.08f, 0.09f, 0.11f, 0.96f));
		const ImGuiWindowFlags menuFlags =
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoNav;
		bool menuVisible = false;
		bool menuHovered = false;
		if (m_ShowOverlaySettingsMenu)
		{
			menuVisible = ImGui::Begin("ViewportOverlaySettingsMenu", &m_ShowOverlaySettingsMenu, menuFlags);
			menuHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
			m_FrameState.IsOverlayHovered |= menuHovered;
			if (menuHovered)
				m_OverlaySettingsKeepAliveUntil = currentTime + 0.20;

			if (menuVisible)
			{
				ImGui::TextUnformatted("Viewport");
				ImGui::Separator();
				ImGui::Checkbox("Show Grid", &m_OverlaySettings.ShowGrid);
				ImGui::SliderFloat("Flight Speed", &m_OverlaySettings.FlightSpeedScale, 0.1f, 8.0f, "%.2f");
				ImGui::SliderFloat("Rotation Speed", &m_OverlaySettings.RotationSpeed, 0.1f, 3.0f, "%.2f");
				ImGui::SliderFloat("Zoom Speed", &m_OverlaySettings.ZoomSpeedScale, 0.1f, 4.0f, "%.2f");

				ImGui::Separator();
				ImGui::TextUnformatted("Grid");
				ImGui::SliderFloat("Minor Cell", &m_OverlaySettings.MinorCellSize, 0.1f, 10.0f, "%.2f");
				ImGui::SliderFloat("Major Cell", &m_OverlaySettings.MajorCellSize, 1.0f, 100.0f, "%.2f");
				ImGui::SliderFloat("Minor Width", &m_OverlaySettings.MinorLineWidth, 0.5f, 4.0f, "%.2f");
				ImGui::SliderFloat("Major Width", &m_OverlaySettings.MajorLineWidth, 0.5f, 6.0f, "%.2f");
				ImGui::SliderFloat("Fade Near", &m_OverlaySettings.FadeNear, 0.0f, 20.0f, "%.2f");
				ImGui::SliderFloat("Fade Far", &m_OverlaySettings.FadeFar, 10.0f, 500.0f, "%.2f");
				ImGui::SliderFloat("Angle Fade", &m_OverlaySettings.FadeAngleStart, 0.001f, 0.2f, "%.3f");
				ImGui::SliderFloat("Depth Bias", &m_OverlaySettings.DepthBias, 0.0001f, 0.05f, "%.4f");
			}

			ImGui::End();
		}
		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);

		const bool keepAlive = currentTime < m_OverlaySettingsKeepAliveUntil;
		if (!settingsHovered && !menuHovered && !keepAlive)
			m_ShowOverlaySettingsMenu = false;

		ImGui::PopStyleColor(3);
		ImGui::PopStyleVar();
	}

	void EditorViewportPanel::RequestPickAtMousePosition()
	{
		const ImVec2 mousePos = ImGui::GetMousePos();
		const float localX = mousePos.x - m_FrameState.BoundsMin.x;
		const float localY = mousePos.y - m_FrameState.BoundsMin.y;

		const uint32_t width = static_cast<uint32_t>(std::max(1.0f, m_FrameState.ViewportSize.x));
		const uint32_t height = static_cast<uint32_t>(std::max(1.0f, m_FrameState.ViewportSize.y));

		const uint32_t pixelX = std::min(
			static_cast<uint32_t>(std::max(0.0f, localX)),
			width - 1);
		const uint32_t pixelY = std::min(
			static_cast<uint32_t>(std::max(0.0f, localY)),
			height - 1);
		const uint32_t flippedPixelY = height - 1 - pixelY;

		m_FrameState.PickRequest = { pixelX, flippedPixelY };
		m_FrameState.WantsPick = true;

		KITA_CORE_INFO(
			"Viewport pick click: screen=({:.2f}, {:.2f}), local=({:.2f}, {:.2f}), viewport=({}, {}), displayPixel=({}, {}), pickingPixel=({}, {})",
			mousePos.x,
			mousePos.y,
			localX,
			localY,
			width,
			height,
			pixelX,
			pixelY,
			pixelX,
			flippedPixelY);
	}

	bool EditorViewportPanel::IsMouseInsideImageBounds() const
	{
		auto [mx, my] = ImGui::GetMousePos();
		return mx >= m_FrameState.BoundsMin.x && mx < m_FrameState.BoundsMax.x
			&& my >= m_FrameState.BoundsMin.y && my < m_FrameState.BoundsMax.y;
	}

}
