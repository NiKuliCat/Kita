#include "kita_pch.h"
#include "LineRenderer.h"
#include <core/Core.h>


namespace Kita {



	LineRenderer::LineRenderer()
	{
		m_ControlPoints.clear();
		m_ControlPoints.push_back({ glm::vec3(0, 0, 0),0 });
		m_ControlPoints.push_back({ glm::vec3(5, 0, 0),1 });
		m_ControlPoints.push_back({ glm::vec3(10, 0, 0),2 });
		m_ControlPoints.push_back({ glm::vec3(15, 0, 0),3 });
		m_ControlPoints.push_back({ glm::vec3(20, 0, 0),4 });
		m_ControlPoints.push_back({ glm::vec3(25, 0, 0),5 });
		m_ControlPoints.push_back({ glm::vec3(30, 0, 0),6 });


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
			if ((m_ControlPoints.size() - 1) % 3 != 0)
			{
				return;
			}
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

	void LineRenderer::MoveControlPoint(int index, const glm::vec3& newPosition)
	{
		if (index < 0 || index >= (int)m_ControlPoints.size())
			return;

		const glm::vec3 oldPosition = m_ControlPoints[index].position;
		const glm::vec3 delta = newPosition - oldPosition;

		if (IsAnchorPoint(index))
		{
			MoveAnchorWithHandles(index, delta);
		}
		else
		{
			m_ControlPoints[index].position = newPosition;
			MirrorOppositeHandle(index);
		}

		m_Dirty = true;
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

		for (size_t start = 0; start + 3 < m_ControlPoints.size(); start += 3)
		{
			const glm::vec3& p0 = m_ControlPoints[start + 0].position;
			const glm::vec3& p1 = m_ControlPoints[start + 1].position;
			const glm::vec3& p2 = m_ControlPoints[start + 2].position;
			const glm::vec3& p3 = m_ControlPoints[start + 3].position;

			for (int i = 0; i <= m_SegmentCountPerBezier; ++i)
			{
				if (start > 0 && i == 0)
					continue;
				const float t = (float)i / (float)m_SegmentCountPerBezier;
				m_CurveVertices.push_back(EvaluateBezierCubic(p0, p1, p2, p3, t));
			}
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

	bool LineRenderer::IsAnchorPoint(int index) const
	{
		return index >= 0 && index < (int)m_ControlPoints.size() && (index % 3 == 0);
	}

	void LineRenderer::MoveAnchorWithHandles(int anchorIndex, const glm::vec3& delta)
	{
		m_ControlPoints[anchorIndex].position += delta;

		const int leftHandle = anchorIndex - 1;
		const int rightHandle = anchorIndex + 1;

		if (leftHandle >= 0)
			m_ControlPoints[leftHandle].position += delta;

		if (rightHandle < (int)m_ControlPoints.size())
			m_ControlPoints[rightHandle].position += delta;
	}

	void LineRenderer::MirrorOppositeHandle(int movedHandleIndex)
	{
		if (movedHandleIndex < 0 || movedHandleIndex >= (int)m_ControlPoints.size())
			return;

		// handle 在 anchor 左边: index % 3 == 2
		if (movedHandleIndex % 3 == 2)
		{
			const int anchorIndex = movedHandleIndex + 1;
			const int oppositeHandle = anchorIndex + 1;

			if (anchorIndex < (int)m_ControlPoints.size() && oppositeHandle < (int)m_ControlPoints.size())
			{
				const glm::vec3 anchor = m_ControlPoints[anchorIndex].position;
				const glm::vec3 moved = m_ControlPoints[movedHandleIndex].position;
				m_ControlPoints[oppositeHandle].position = anchor + (anchor - moved);
			}
		}
		// handle 在 anchor 右边: index % 3 == 1
		else if (movedHandleIndex % 3 == 1)
		{
			const int anchorIndex = movedHandleIndex - 1;
			const int oppositeHandle = anchorIndex - 1;

			if (anchorIndex >= 0 && oppositeHandle >= 0)
			{
				const glm::vec3 anchor = m_ControlPoints[anchorIndex].position;
				const glm::vec3 moved = m_ControlPoints[movedHandleIndex].position;
				m_ControlPoints[oppositeHandle].position = anchor + (anchor - moved);
			}
		}
	}

}
