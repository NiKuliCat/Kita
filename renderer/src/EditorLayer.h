#pragma once
#include <EngineCore.h>
#include "scene/EditorRenderer.h"
#include "ui/InspectorPanel.h"
#include "ui/SceneHierarchyPanel.h"
#include "ui/EditorSelectionContext.h"
#include "ui/ContentBrowserPanel.h"
#include "ui/SvgIconAtlas.h"
#include "ui/ThumbnailCache.h"
#include "ui/UIColorPanel.h"
#include "ui/viewport/ViewportInstance.h"
namespace Kita {

	class EditorLayer :public Layer{

	public:
		EditorLayer();


		virtual ~EditorLayer() = default;


		virtual void OnCreate()  override;
		virtual void OnUpdate(Timestep ts)  override;
		virtual void OnDestroy()  override;

		virtual void OnRender()  override;
		virtual void OnImGuiRender()  override;
		virtual void OnEvent(Event& event)  override;

	private:
		bool OnKeyPressed(KeyPressedEvent& event);
		bool OnMouseButtonPressed(MouseButtonPressedEvent& event);
		std::string BuildNextViewportWindowName();
		void AddViewportPanel(std::string windowName);
		void RemoveClosedViewportPanels();
		void RenderTimeSystemPanel();
	private:

		Ref<Scene> m_Scene = nullptr;
		SceneSerializer  m_SceneSerializer;

		//ui
		SceneHierarchyPanel m_SceneHierarchyPanel;
		InspectorPanel m_InspectorPanel;
		ContentBrowserPanel m_ContentBrowserPanel;
		UIColorPanel m_UIColorPanel;
		std::vector<ViewportInstance> m_SceneViewportPanels{};
		int32_t m_ActiveViewportIndex = -1;
		uint32_t m_NextViewportSerial = 1;
		bool m_ShowTimeSystemPanel = true;

		Ref<EditorSelectionContext> m_EditorSelectionContext = nullptr;
		Unique<VulkanResourceFactory> m_EditorVulkanResourceFactory = nullptr;
		Unique<PipelineFactory> m_PipelineFactory = nullptr;
		Unique<ThumbnailCache> m_ContentBrowserThumbnailCache = nullptr;
		Unique<SvgIconAtlas> m_ContentBrowserIconAtlas = nullptr;
	};
}
