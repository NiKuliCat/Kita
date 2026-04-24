#pragma once
#include <glm/glm.hpp>

namespace Kita {

	enum class CurveType
	{
		Polyline = 0,
		BezierCubic
	};

	struct PointData
	{
		glm::vec3 position;
		glm::vec4 color;
		int id = -1;

		PointData() = default;
		PointData(const glm::vec3& pos, const int index)
			:position(pos),id(index),color(glm::vec4(1,1,1,1))
		{

		}
	};


	class LineRenderer
	{
	public:
		LineRenderer();
		~LineRenderer();

		void SetControlPoints(const std::vector<glm::vec3>& points);
		std::vector<PointData>& GetControlPoints() { return m_ControlPoints; }
		const std::vector<PointData>& GetControlPoints() const { return m_ControlPoints; }

		const PointData& GetControlPointByIndex(const int index) const;
		PointData& GetControlPointByIndex(const int index);

		void SetControlPointColorByIndex(const glm::vec4& color, const int index);

		void SetCurveType(CurveType type) { m_CurveType = type; }
		CurveType GetCurveType() const { return m_CurveType; }



		void RebuildIfNeeded();





	private:

		std::vector<PointData> m_ControlPoints;

		CurveType m_CurveType = CurveType::BezierCubic;
	};
}
