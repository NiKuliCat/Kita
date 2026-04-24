#pragma once
#include <glm/glm.hpp>

namespace Kita {

	enum class CurveType
	{
		Polyline = 0,
		BezierCubic
	};


	class LineRenderer
	{
	public:
		LineRenderer();
		~LineRenderer();

		void SetControlPoints(const std::vector<glm::vec3>& points) { m_ControlPoints = points; }
		std::vector<glm::vec3>& GetControlPoints() { return m_ControlPoints; }
		const std::vector<glm::vec3>& GetControlPoints() const { return m_ControlPoints; }

		void SetCurveType(CurveType type) { m_CurveType = type; }
		CurveType GetCurveType() const { return m_CurveType; }



		void RebuildIfNeeded();





	private:

		std::vector<glm::vec3> m_ControlPoints;

		CurveType m_CurveType = CurveType::BezierCubic;
	};
}
