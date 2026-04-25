#pragma once

#include <Engine.h>

namespace Kita {

	class SceneSelectionContext
	{
	public:
		void SetSelection(Object object)
		{
			if (m_SelectedObject == object)
				return;

			ClearSelectedPoint();
			m_SelectedObject = object;
		}

		void SetSelectedPoint(PointData point)
		{
			m_SelectedPoint = point;
		}

		Object& GetSelectedObject() { return m_SelectedObject; }
		const Object& GetSelectedObject() const { return m_SelectedObject; }

		PointData& GetSelectedPoint() { return m_SelectedPoint; }
		const PointData& GetSelectedPoint() const { return m_SelectedPoint; }

		void ClearSelectedPoint()
		{
			if (m_SelectedObject && m_SelectedPoint.id != -1 && m_SelectedObject.HasComponent<LineRenderer>())
			{
				auto& lineRenderer = m_SelectedObject.GetComponent<LineRenderer>();
				lineRenderer.ResetControlPointVisual(m_SelectedPoint.id);
				lineRenderer.ClearSelectedControlPoint();
			}

			m_SelectedPoint = {};
		}

		void ClearSelection()
		{
			ClearSelectedPoint();
			m_SelectedObject = {};
		}

	private:
		Object m_SelectedObject;
		PointData m_SelectedPoint;
	};
}
