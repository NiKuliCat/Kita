#include "kita_pch.h"
#include "LineRenderer.h"


namespace Kita {



	LineRenderer::LineRenderer()
	{
		m_ControlPoints.clear();
		m_ControlPoints.push_back(glm::vec3(1, 1, 1));
		m_ControlPoints.push_back(glm::vec3(2, 1, 1));
		m_ControlPoints.push_back(glm::vec3(3, 1, 1));
	}

	LineRenderer::~LineRenderer()
	{
	}

	void LineRenderer::RebuildIfNeeded()
	{
	}

}