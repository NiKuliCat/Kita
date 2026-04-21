#pragma once
#include <Engine.h>

namespace Kita {


	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene)
			:m_SceneContext(scene) { m_SelectedObject = {}; }

		void SetContext(const Ref<Scene>& scene) { m_SceneContext = scene;  	m_SelectedObject = {}; }
		void SetSelectedObject(Object obj);
		Object GetSelectedObject() { return m_SelectedObject; }
		void OnImGuiRender();
		operator bool() const { return !(m_SceneContext == nullptr); }
	private:
		void DrawObjectNode(Object obj);
		void DrawInspectorPanel();
		void OnSlectedObjectChange(Object obj);

	private:
		Ref<Scene> m_SceneContext = nullptr;
		Object  m_SelectedObject;
	};

}