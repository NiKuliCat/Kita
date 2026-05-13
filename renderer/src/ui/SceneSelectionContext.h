#pragma once

#include <EngineCore.h>

namespace Kita {

	struct SelectedPoint
	{
		int id = -1;
	};

	enum class CurveType : uint32_t
	{
		Polyline = 0,
		BezierCubic
	};

	enum class BezierHandleMode : uint32_t
	{
		Free = 0,
		Aligned,
		Mirrored
	};

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

		void SetSelectedPoint(SelectedPoint point)
		{
			m_SelectedPoint = point;
		}

		Object& GetSelectedObject() { return m_SelectedObject; }
		const Object& GetSelectedObject() const { return m_SelectedObject; }

		SelectedPoint& GetSelectedPoint() { return m_SelectedPoint; }
		const SelectedPoint& GetSelectedPoint() const { return m_SelectedPoint; }

		void ClearSelectedPoint()
		{
			m_SelectedPoint = {};
		}

		void ClearSelection()
		{
			ClearSelectedPoint();
			m_SelectedObject = {};
		}

	private:
		Object m_SelectedObject;
		SelectedPoint m_SelectedPoint;
	};
}
