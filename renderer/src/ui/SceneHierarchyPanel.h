#pragma once
#include <Engine.h>
#include "SceneSelectionContext.h"

namespace Kita {


	class SceneHierarchyPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene, const Ref<SceneSelectionContext>& selectionContext)
			:m_SceneContext(scene), m_SelectionContext(selectionContext) {}

		void SetContext(const Ref<Scene>& scene) { m_SceneContext = scene; }
		void SetSelectionContext(const Ref<SceneSelectionContext>& selectionContext) { m_SelectionContext = selectionContext; }

		Object& GetSelectedObject()
		{
			static Object emptyObject;
			return m_SelectionContext ? m_SelectionContext->GetSelectedObject() : emptyObject;
		}
		const Object& GetSelectedObject() const
		{
			static Object emptyObject;
			return m_SelectionContext ? m_SelectionContext->GetSelectedObject() : emptyObject;
		}

		PointData& GetSelectedPoint()
		{
			static PointData emptyPoint{};
			return m_SelectionContext ? m_SelectionContext->GetSelectedPoint() : emptyPoint;
		}
		const PointData& GetSelectedPoint() const
		{
			static PointData emptyPoint{};
			return m_SelectionContext ? m_SelectionContext->GetSelectedPoint() : emptyPoint;
		}

		void SetSelectedObject(Object obj) { if (m_SelectionContext) m_SelectionContext->SetSelection(obj); }
		void SetSelectedPoint(PointData point) { if (m_SelectionContext) m_SelectionContext->SetSelectedPoint(point); }

		void ClearSelectedPoint() { if (m_SelectionContext) m_SelectionContext->ClearSelectedPoint(); }
		void ClearSelection() { if (m_SelectionContext) m_SelectionContext->ClearSelection(); }

		void OnImGuiRender();
		operator bool() const { return !(m_SceneContext == nullptr); }
	private:
		void DrawObjectNode(Object obj);
		void OnSlectedObjectChange(Object obj);

	private:
		Ref<Scene> m_SceneContext = nullptr;
		Ref<SceneSelectionContext> m_SelectionContext = nullptr;
	};

}
