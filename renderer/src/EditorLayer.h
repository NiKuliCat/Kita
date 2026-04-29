#pragma once
#include <Engine.h>
#include "scene/ViewportCamera.h"
#include "ui/SceneHierarchyPanel.h"
#include "ui/SceneSelectionContext.h"
#include "ui/SceneViewportPanel.h"
#include "ui/ContentBrowserPanel.h"
#include "ui/UIColorPanel.h"
namespace Kita {

	class EditorLayer :public Layer{

	public:
		EditorLayer();


		virtual ~EditorLayer() = default;


		virtual void OnCreate()  override;
		virtual void OnUpdate(float daltaTime)  override;
		virtual void OnDestroy()  override;

		virtual void OnImGuiRender()  override;
		virtual void OnEvent(Event& event)  override;

	private:
		bool OnKeyPressed(KeyPressedEvent& event);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
		void AddViewportPanel(std::string windowName);
		void RemoveClosedViewportPanels();
	private:

		Ref<Scene> m_Scene = nullptr;
		SceneSerializer  m_SceneSerializer;

		//ui
		SceneHierarchyPanel m_SceneHierarchyPanel;
		ContentBrowserPanel m_ContentBrowserPanel;
		UIColorPanel m_UIColorPanel;
		std::vector<Unique<SceneViewportPanel>> m_SceneViewportPanels{};
		int32_t m_ActiveViewportIndex = -1;
		uint32_t m_NextViewportSerial = 1;

		Ref<SceneSelectionContext> m_SceneSelectionContext = nullptr;
	};
}
