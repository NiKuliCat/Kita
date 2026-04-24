#pragma once
#include <Engine.h>

namespace Kita {


	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene)
			:m_SceneContext(scene) {
			m_SelectedObject = {}; m_SelectedPoint = {};
		}

		void SetContext(const Ref<Scene>& scene) { m_SceneContext = scene;  	m_SelectedObject = {}; }
		void SetSelectedObject(Object obj);
		void SetSelectedPoint(PointData point);

		Object& GetSelectedObject() { return m_SelectedObject; }
		const Object& GetSelectedObject() const { return m_SelectedObject; }

		PointData& GetSelectedPoint() { return m_SelectedPoint; }
		const PointData& GetSelectedPoint() const { return m_SelectedPoint; }


		void ClearSelectedPoint();
		void ClearSelection();

		void OnImGuiRender();
		operator bool() const { return !(m_SceneContext == nullptr); }
	private:
		void DrawObjectNode(Object obj);
		void DrawInspectorPanel();
		void OnSlectedObjectChange(Object obj);

	private:
		Ref<Scene> m_SceneContext = nullptr;
		Object  m_SelectedObject;
		PointData m_SelectedPoint;
	};

}
