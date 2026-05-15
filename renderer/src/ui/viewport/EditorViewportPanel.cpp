#include "renderer_pch.h"
#include "EditorViewportPanel.h"

#include "imgui.h"
#include "ImGuizmo.h"
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
		: m_WindowName(std::move(windowName)),m_ViewportCamera(CreateUnique<ViewportCamera>())
	{
		m_GizmoControlType = ImGuizmo::OPERATION::TRANSLATE;
	}

	void EditorViewportPanel::OnUpdata(Timestep ts)
	{
		if (!m_ViewportCamera || !m_IsActive)
			return;

		m_ViewportCamera->OnUpdate(ts.GetSecondsF());
	}

	void EditorViewportPanel::OnImGuiRender()
	{
		if (!m_IsOpen)
			return;

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
				m_IsOpen = false;
				m_IsHovered = false;
				m_IsFocused = false;
				m_IsImageHovered = false;
			}
			ImGui::EndPopup();
		}

		if (!m_IsOpen)
		{
			ImGui::End();
			ImGui::PopStyleVar();
			return;
		}

		m_IsHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);
		m_IsFocused = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
		m_IsOverlayHovered = false;

		const ImVec2 contentSize = ImGui::GetContentRegionAvail();
		if (contentSize.x > 1.0f && contentSize.y > 1.0f)
		{
			m_ViewportSize = { contentSize.x, contentSize.y };
			if (m_ViewportCamera)
				m_ViewportCamera->SetViewport(m_ViewportSize.x, m_ViewportSize.y);
		}

		const ImVec2 imageSize(
			std::max(1.0f, m_ViewportSize.x),
			std::max(1.0f, m_ViewportSize.y));

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

		m_IsImageHovered = ImGui::IsItemHovered();
		const ImVec2 itemMin = ImGui::GetItemRectMin();
		const ImVec2 itemMax = ImGui::GetItemRectMax();
		m_ViewportBounds[0] = { itemMin.x, itemMin.y };
		m_ViewportBounds[1] = { itemMax.x, itemMax.y };
		DrawViewportOverlay();
		if (m_IsOverlayHovered)
			m_IsImageHovered = false;

		if (m_IsActive &&
			m_IsImageHovered &&
			ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
			!ImGuizmo::IsOver())
		{
			RequestPickAtMousePosition();
		}

		ImGui::End();
		ImGui::PopStyleVar();
	}

	void EditorViewportPanel::OnEvent(Event& event)
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
		dispatcher.Dispatcher<KeyPressedEvent>(BIND_EVENT_FUNC(EditorViewportPanel::OnKeyPressed));
		dispatcher.Dispatcher<MouseButtonPressedEvent>(BIND_EVENT_FUNC(EditorViewportPanel::OnMouseButtonPressed));
		m_ViewportCamera->OnEvent(event);
	}

	ViewportPickRequest EditorViewportPanel::ConsumePickRequest()
	{
		ViewportPickRequest request = m_PendingPickRequest;
		m_PendingPickRequest = {};
		m_HasPendingPickRequest = false;
		return request;
	}

	bool EditorViewportPanel::OnKeyPressed(KeyPressedEvent& event)
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

	bool EditorViewportPanel::OnMouseButtonPressed(MouseButtonPressedEvent& event)
	{
		return false;
	}

	void EditorViewportPanel::DrawViewportOverlay()
	{
		ImGui::SetCursorScreenPos(ImVec2(
			m_ViewportBounds[0].x + kOverlayPadding,
			m_ViewportBounds[0].y + kOverlayPadding));

		ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 6.0f);
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.11f, 0.13f, 0.92f));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.20f, 0.24f, 0.96f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.22f, 0.24f, 0.28f, 0.98f));

		if (ImGui::Button(m_OverlaySettings.ShowGrid ? "[#]" : "[ ]", ImVec2(kOverlayButtonSize, kOverlayButtonSize)))
			m_OverlaySettings.ShowGrid = !m_OverlaySettings.ShowGrid;
		m_IsOverlayHovered |= ImGui::IsItemHovered();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Toggle Grid");

		ImGui::SameLine(0.0f, 6.0f);
		ImGui::Button("[=]", ImVec2(kOverlayButtonSize, kOverlayButtonSize));
		const bool settingsHovered = ImGui::IsItemHovered();
		m_IsOverlayHovered |= settingsHovered;
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
			m_IsOverlayHovered |= menuHovered;
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
		const float localX = mousePos.x - m_ViewportBounds[0].x;
		const float localY = mousePos.y - m_ViewportBounds[0].y;

		const uint32_t width = static_cast<uint32_t>(std::max(1.0f, m_ViewportSize.x));
		const uint32_t height = static_cast<uint32_t>(std::max(1.0f, m_ViewportSize.y));

		const uint32_t pixelX = std::min(
			static_cast<uint32_t>(std::max(0.0f, localX)),
			width - 1);
		const uint32_t pixelY = std::min(
			static_cast<uint32_t>(std::max(0.0f, localY)),
			height - 1);

		m_PendingPickRequest = { pixelX, pixelY };
		m_HasPendingPickRequest = true;
	}

	bool EditorViewportPanel::IsMouseInsideImageBounds() const
	{
		auto [mx, my] = ImGui::GetMousePos();
		return mx >= m_ViewportBounds[0].x && mx < m_ViewportBounds[1].x
			&& my >= m_ViewportBounds[0].y && my < m_ViewportBounds[1].y;
	}

}
