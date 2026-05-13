#pragma once

#include <EngineCore.h>
#include "scene/EditorRenderer.h"
#include "ui/viewport/EditorViewportPanel.h"
#include "ui/viewport/EditorViewportSurface.h"

namespace Kita {

	class ViewportInstance
	{
	public:
		ViewportInstance() = default;
		ViewportInstance(VulkanContext& context, const Ref<Scene>& scene, std::string windowName = "Viewport");

		ViewportInstance(const ViewportInstance&) = delete;
		ViewportInstance& operator=(const ViewportInstance&) = delete;

		ViewportInstance(ViewportInstance&&) noexcept = default;
		ViewportInstance& operator=(ViewportInstance&&) noexcept = default;

		~ViewportInstance() = default;

		void OnUpdate(Timestep ts);
		void OnRender();
		void OnImGuiRender();
		void OnEvent(Event& event);

		void SetActive(bool isActive);

		bool IsOpen() const;
		bool IsImageHovered() const;
		bool IsWindowFocused() const;

		EditorViewportPanel* GetPanel() const { return m_Panel.get(); }
		EditorViewportSurface* GetSurface() const { return m_Surface.get(); }
		EditorRenderer* GetRenderer() const { return m_Renderer.get(); }

	private:
		Unique<EditorViewportPanel> m_Panel = nullptr;
		Unique<EditorViewportSurface> m_Surface = nullptr;
		Unique<EditorRenderer> m_Renderer = nullptr;
	};

}
