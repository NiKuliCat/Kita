#include "kita_pch.h"
#include "LineRenderer.h"
#include <core/Core.h>


namespace Kita {



	LineRenderer::LineRenderer()
	{
		m_ControlPoints.clear();
		m_ControlPoints.push_back({ glm::vec3(1, 1, 1),0 });
		m_ControlPoints.push_back({ glm::vec3(2, 1, 1),1 });
		m_ControlPoints.push_back({ glm::vec3(3, 1, 1),2 });

	}

	LineRenderer::~LineRenderer()
	{
	}

	void LineRenderer::SetControlPoints(const std::vector<glm::vec3>& points)
	{
		m_ControlPoints.clear();
		for (int i = 0; i < points.size(); i++)
		{
			m_ControlPoints.push_back({ points[i],i });
		}
	}

	const PointData& LineRenderer::GetControlPointByIndex(const int index) const
	{
		return m_ControlPoints[index];
	}

	PointData& LineRenderer::GetControlPointByIndex(const int index)
	{
		return m_ControlPoints[index];
	}

	void LineRenderer::SetControlPointColorByIndex(const glm::vec4& color,const int index)
	{
		m_ControlPoints[index].color = color;	
	}

	void LineRenderer::RebuildIfNeeded()
	{
	}

}