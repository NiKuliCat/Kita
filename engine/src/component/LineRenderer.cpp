#include "kita_pch.h"
#include "LineRenderer.h"
#include <core/Core.h>


namespace Kita {



	LineRenderer::LineRenderer()
	{
		m_ControlPoints.clear();
		m_ControlPoints.push_back({ glm::vec3(1, 1, 1),0 });
		m_ControlPoints.push_back({ glm::vec3(5, 1, 1),1 });
		m_ControlPoints.push_back({ glm::vec3(9, 1, 1),2 });
		m_ControlPoints.push_back({ glm::vec3(13, 1, 1),3 });


		InitBuffer();
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
		m_Dirty = true;
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
		if (!m_Dirty) return;

		m_CurveVertices.clear();
		m_CurveVertexCount = 0;

		if (m_CurveType == CurveType::BezierCubic)
		{
			BuildBezierCubic();
		}

		m_CurveVertexCount = (uint32_t)m_CurveVertices.size();

		if (m_CurveVertexCount > 0)
		{
			ReCreateBuffer(m_CurveVertexCount);
			m_Curve_VBO->SetData(
				m_CurveVertices.data(),
				m_CurveVertexCount * (uint32_t)sizeof(glm::vec3),
				0);
		}

		m_Dirty = false;
	}

	glm::vec3 LineRenderer::EvaluateBezierCubic(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t)
	{
		float u = 1.0f - t;
		float tt = t * t;
		float uu = u * u;
		float uuu = uu * u;
		float ttt = tt * t;

		return uuu * p0 + 3.0f * uu * t * p1 + 3.0f * u * tt * p2+ ttt * p3;
	}

	void LineRenderer::BuildBezierCubic()
	{
		if (m_ControlPoints.size() < 4)
			return;

		const glm::vec3& p0 = m_ControlPoints[0].position;
		const glm::vec3& p1 = m_ControlPoints[1].position;
		const glm::vec3& p2 = m_ControlPoints[2].position;
		const glm::vec3& p3 = m_ControlPoints[3].position;

		for (int i = 0; i <= m_SegmentCountPerBezier; ++i)
		{
			const float t = (float)i / (float)m_SegmentCountPerBezier;
			m_CurveVertices.push_back(EvaluateBezierCubic(p0, p1, p2, p3, t));
		}
	}

	void LineRenderer::InitBuffer()
	{
		m_CurveVertexCapacity = static_cast<uint32_t>(m_SegmentCountPerBezier + 1);
		m_CurveVertexLayout ={{ ShaderDataType::Float3, "PositionOS" }};

		m_Curve_VAO = VertexArray::Create();
		m_Curve_VBO = VertexBuffer::Create(m_CurveVertexCapacity * (uint32_t)sizeof(glm::vec3));

		m_Curve_VBO->SetLayout(m_CurveVertexLayout);
		m_Curve_VAO->AddVertexBuffer(m_Curve_VBO);
	}



	void LineRenderer::ReCreateBuffer(const uint32_t size)
	{
		auto count = std::max(1u, size);

		if (m_Curve_VAO && m_Curve_VBO && count <= m_CurveVertexCapacity)
			return;

		m_CurveVertexCapacity = count;

		m_Curve_VAO = VertexArray::Create();
		m_Curve_VBO = VertexBuffer::Create(m_CurveVertexCapacity * (uint32_t)sizeof(glm::vec3));


		m_Curve_VBO->SetLayout(m_CurveVertexLayout);
		m_Curve_VAO->AddVertexBuffer(m_Curve_VBO);
	}

}
