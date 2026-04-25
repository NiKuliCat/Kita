#include "kita_pch.h"
#include "LineRenderer.h"

#include <core/Core.h>
#include "render/Renderer.h"
#include "render/ShaderLibrary.h"
namespace Kita {

	LineRenderer::LineRenderer()
	{
		m_ControlPoints.clear();
		m_ControlPoints.push_back({ glm::vec3(0, 0, 0), 0 });
		m_ControlPoints.push_back({ glm::vec3(5, 0, 0), 1 });
		m_ControlPoints.push_back({ glm::vec3(10, 0, 0), 2 });
		m_ControlPoints.push_back({ glm::vec3(15, 0, 0), 3 });
		m_ControlPoints.push_back({ glm::vec3(20, 0, 0), 4 });
		m_ControlPoints.push_back({ glm::vec3(25, 0, 0), 5 });
		m_ControlPoints.push_back({ glm::vec3(30, 0, 0), 6 });

		ResizeHandleModesToMatchAnchors();
		ResetAllControlPointVisuals();
		InitBuffer();
		InitEditorHelperBuffers();
	}

	LineRenderer::~LineRenderer()
	{
	}

	void LineRenderer::SetControlPoints(const std::vector<glm::vec3>& points)
	{
		m_ControlPoints.clear();
		for (int i = 0; i < (int)points.size(); ++i)
		{
			m_ControlPoints.push_back({ points[i], i });
		}

		ResizeHandleModesToMatchAnchors();
		ResetAllControlPointVisuals();
		m_Dirty = true;
		m_HelperDirty = true;
	}

	const PointData& LineRenderer::GetControlPointByIndex(const int index) const
	{
		return m_ControlPoints[index];
	}

	PointData& LineRenderer::GetControlPointByIndex(const int index)
	{
		return m_ControlPoints[index];
	}

	void LineRenderer::SetControlPointColorByIndex(const glm::vec4& color, const int index)
	{
		m_ControlPoints[index].color = color;
		m_HelperDirty = true;
	}

	uint32_t LineRenderer::GetBezierSegmentCount() const
	{
		if (m_ControlPoints.size() < 4 || (m_ControlPoints.size() - 1) % 3 != 0)
			return 0;

		return static_cast<uint32_t>((m_ControlPoints.size() - 1) / 3);
	}

	bool LineRenderer::IsAnchorControlPoint(int index) const
	{
		return IsAnchorPoint(index);
	}

	int LineRenderer::GetAnchorIndexForControlPoint(int index) const
	{
		if (index < 0 || index >= (int)m_ControlPoints.size())
			return -1;

		if (IsAnchorPoint(index))
			return index;

		if (index % 3 == 1)
			return index - 1;

		if (index % 3 == 2)
			return index + 1;

		return -1;
	}

	int LineRenderer::GetLeftHandleIndexForAnchor(int anchorIndex) const
	{
		if (!IsAnchorPoint(anchorIndex))
			return -1;

		const int handleIndex = anchorIndex - 1;
		return handleIndex >= 0 ? handleIndex : -1;
	}

	int LineRenderer::GetRightHandleIndexForAnchor(int anchorIndex) const
	{
		if (!IsAnchorPoint(anchorIndex))
			return -1;

		const int handleIndex = anchorIndex + 1;
		return handleIndex < (int)m_ControlPoints.size() ? handleIndex : -1;
	}

	BezierHandleMode LineRenderer::GetHandleModeForPoint(int index) const
	{
		return GetHandleModeForAnchor(GetAnchorIndexForControlPoint(index));
	}

	void LineRenderer::SetHandleModeForPoint(int index, BezierHandleMode mode)
	{
		const int anchorIndex = GetAnchorIndexForControlPoint(index);
		if (anchorIndex == -1)
			return;

		SetHandleModeForAnchor(anchorIndex, mode);

		if (IsAnchorPoint(index))
		{
			const int leftHandle = GetLeftHandleIndexForAnchor(anchorIndex);
			const int rightHandle = GetRightHandleIndexForAnchor(anchorIndex);

			if (rightHandle != -1)
			{
				ApplyHandleMode(rightHandle);
			}
			else if (leftHandle != -1)
			{
				ApplyHandleMode(leftHandle);
			}
			return;
		}

		ApplyHandleMode(index);
	}

	void LineRenderer::AppendBezierSegment()
	{
		if (!m_ControlPoints.empty() && (m_ControlPoints.size() - 1) % 3 != 0)
			return;

		if (m_ControlPoints.empty())
		{
			m_ControlPoints.push_back({ glm::vec3(0.0f, 0.0f, 0.0f), 0 });
			m_ControlPoints.push_back({ glm::vec3(3.0f, 0.0f, 0.0f), 1 });
			m_ControlPoints.push_back({ glm::vec3(6.0f, 0.0f, 0.0f), 2 });
			m_ControlPoints.push_back({ glm::vec3(9.0f, 0.0f, 0.0f), 3 });
		}
		else
		{
			const glm::vec3 lastAnchor = m_ControlPoints.back().position;
			glm::vec3 outDirection = glm::vec3(3.0f, 0.0f, 0.0f);

			if (m_ControlPoints.size() >= 2)
			{
				outDirection = lastAnchor - m_ControlPoints[m_ControlPoints.size() - 2].position;
				if (glm::dot(outDirection, outDirection) < 0.000001f)
				{
					outDirection = glm::vec3(3.0f, 0.0f, 0.0f);
				}
			}

			const int nextId = static_cast<int>(m_ControlPoints.size());
			m_ControlPoints.push_back({ lastAnchor + outDirection, nextId + 0 });
			m_ControlPoints.push_back({ lastAnchor + outDirection * 2.0f, nextId + 1 });
			m_ControlPoints.push_back({ lastAnchor + outDirection * 3.0f, nextId + 2 });
		}

		ResizeHandleModesToMatchAnchors();
		m_Dirty = true;
		m_HelperDirty = true;
	}

	void LineRenderer::RemoveLastBezierSegment()
	{
		if (m_ControlPoints.size() <= 4)
			return;

		m_ControlPoints.resize(m_ControlPoints.size() - 3);
		for (int i = 0; i < (int)m_ControlPoints.size(); ++i)
		{
			m_ControlPoints[i].id = i;
		}

		ResizeHandleModesToMatchAnchors();
		ResetAllControlPointVisuals();
		ValidateSelectedControlPoint();
		m_Dirty = true;
		m_HelperDirty = true;
	}

	glm::vec4 LineRenderer::GetDefaultControlPointColor(int index) const
	{
		if (IsAnchorPoint(index))
		{
			return glm::vec4(0.96f, 0.64f, 0.18f, 1.0f);
		}

		return glm::vec4(0.74f, 0.80f, 0.90f, 1.0f);
	}

	float LineRenderer::GetControlPointRadius(int index) const
	{
		return IsAnchorPoint(index) ? 7.5f : 5.0f;
	}

	void LineRenderer::ResetControlPointVisual(int index)
	{
		if (index < 0 || index >= (int)m_ControlPoints.size())
			return;

		m_ControlPoints[index].color = GetDefaultControlPointColor(index);
		m_HelperDirty = true;
	}

	void LineRenderer::ResetAllControlPointVisuals()
	{
		for (int i = 0; i < (int)m_ControlPoints.size(); ++i)
		{
			ResetControlPointVisual(i);
		}
	}

	void LineRenderer::RebuildIfNeeded()
	{
		if (!m_Dirty)
			return;

		m_CurveVertices.clear();
		m_CurveVertexCount = 0;

		if (m_CurveType == CurveType::BezierCubic)
		{
			if ((m_ControlPoints.size() - 1) % 3 != 0)
			{
				m_Dirty = false;
				return;
			}
			BuildBezierCubic();
		}

		m_CurveVertexCount = static_cast<uint32_t>(m_CurveVertices.size());

		if (m_CurveVertexCount > 0)
		{
			ReCreateBuffer(m_CurveVertexCount);
			m_Curve_VBO->SetData(
				m_CurveVertices.data(),
				m_CurveVertexCount * static_cast<uint32_t>(sizeof(glm::vec3)),
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
			ApplyHandleMode(index);
		}

		m_Dirty = true;
		m_HelperDirty = true;
	}

	void LineRenderer::SetSelectedControlPoint(int index)
	{
		if (index < 0 || index >= static_cast<int>(m_ControlPoints.size()))
		{
			ClearSelectedControlPoint();
			return;
		}

		m_SelectedControlPointIndex = index;
		m_SelectedAnchorIndex = GetAnchorIndexForControlPoint(index);
		m_HelperDirty = true;
	}

	void LineRenderer::ClearSelectedControlPoint()
	{
		m_SelectedControlPointIndex = -1;
		m_SelectedAnchorIndex = -1;
		m_HelperDirty = true;
	}

	void LineRenderer::RenderEditorHelpers(const glm::mat4& model, uint32_t objectId)
	{
		RebuildEditorHelpersIfNeeded();

		auto& shaderLibrary = ShaderLibrary::GetInstance();
		if (m_HelperLineVertices.size() >= 2)
		{
			auto lineShader = shaderLibrary.Get("EditorLineShader");
			lineShader->SetMat4("Model", model);
			lineShader->SetInt("id", objectId);
			lineShader->SetColor("Color", glm::vec4(0.92f, 0.94f, 0.98f, 0.28f));
			for (uint32_t offset = 0; offset + 1 < static_cast<uint32_t>(m_HelperLineVertices.size()); offset += 2)
			{
				m_HelperLineVBO->SetData(&m_HelperLineVertices[offset], static_cast<uint32_t>(sizeof(glm::vec3) * 2), 0);
				Renderer::SubmitAsLine(m_HelperLineVAO, lineShader, 2, 1.5f);
			}
		}

		if (!m_HandlePointVertices.empty())
		{
			auto handleShader = shaderLibrary.Get("GizmoDiamond");
			handleShader->SetMat4("Matrix_M", model);
			handleShader->SetInt("id", objectId);
			Renderer::DrawGizmoPoints(m_HandlePointVAO, handleShader, static_cast<uint32_t>(m_HandlePointVertices.size()));
		}
	}

	glm::vec3 LineRenderer::EvaluateBezierCubic(
		const glm::vec3& p0,
		const glm::vec3& p1,
		const glm::vec3& p2,
		const glm::vec3& p3,
		float t)
	{
		const float u = 1.0f - t;
		const float tt = t * t;
		const float uu = u * u;
		const float uuu = uu * u;
		const float ttt = tt * t;

		return uuu * p0 + 3.0f * uu * t * p1 + 3.0f * u * tt * p2 + ttt * p3;
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
		m_CurveVertexLayout = { { ShaderDataType::Float3, "PositionOS" } };

		m_Curve_VAO = VertexArray::Create();
		m_Curve_VBO = VertexBuffer::Create(m_CurveVertexCapacity * static_cast<uint32_t>(sizeof(glm::vec3)));

		m_Curve_VBO->SetLayout(m_CurveVertexLayout);
		m_Curve_VAO->AddVertexBuffer(m_Curve_VBO);
	}

	void LineRenderer::InitEditorHelperBuffers()
	{
		m_HelperLineLayout = { { ShaderDataType::Float3, "PositionOS" } };
		m_HandlePointLayout = {
			{ ShaderDataType::Float3, "position" },
			{ ShaderDataType::Float4, "color" },
			{ ShaderDataType::Float, "radius" },
			{ ShaderDataType::Int, "index" }
		};

		ReCreateHelperLineBuffer(2);
		ReCreateHandlePointBuffer(1);
	}

	void LineRenderer::ReCreateBuffer(const uint32_t size)
	{
		auto count = std::max(1u, size);

		if (m_Curve_VAO && m_Curve_VBO && count <= m_CurveVertexCapacity)
			return;

		m_CurveVertexCapacity = count;

		m_Curve_VAO = VertexArray::Create();
		m_Curve_VBO = VertexBuffer::Create(m_CurveVertexCapacity * static_cast<uint32_t>(sizeof(glm::vec3)));

		m_Curve_VBO->SetLayout(m_CurveVertexLayout);
		m_Curve_VAO->AddVertexBuffer(m_Curve_VBO);
	}

	void LineRenderer::ReCreateHelperLineBuffer(uint32_t vertexCount)
	{
		const uint32_t requiredCount = std::max(2u, vertexCount);
		if (m_HelperLineVAO && m_HelperLineVBO && requiredCount <= m_HelperLineCapacity)
			return;

		m_HelperLineCapacity = requiredCount;
		m_HelperLineVAO = VertexArray::Create();
		m_HelperLineVBO = VertexBuffer::Create(m_HelperLineCapacity * static_cast<uint32_t>(sizeof(glm::vec3)));
		m_HelperLineVBO->SetLayout(m_HelperLineLayout);
		m_HelperLineVAO->AddVertexBuffer(m_HelperLineVBO);
	}

	void LineRenderer::ReCreateHandlePointBuffer(uint32_t pointCount)
	{
		const uint32_t requiredBytes = std::max(1u, pointCount) * static_cast<uint32_t>(sizeof(EditorHandlePointData));
		if (m_HandlePointVAO && m_HandlePointVBO && requiredBytes <= m_HandlePointCapacityBytes)
			return;

		m_HandlePointCapacityBytes = std::max(requiredBytes, 1024u);
		m_HandlePointVAO = VertexArray::Create();
		m_HandlePointVBO = VertexBuffer::Create(m_HandlePointCapacityBytes);
		m_HandlePointVBO->SetLayout(m_HandlePointLayout);
		m_HandlePointVAO->AddVertexBuffer(m_HandlePointVBO);
	}

	void LineRenderer::RebuildEditorHelpersIfNeeded()
	{
		if (!m_HelperDirty)
			return;

		ValidateSelectedControlPoint();
		BuildSelectedHelperGeometry();

		ReCreateHelperLineBuffer(static_cast<uint32_t>(m_HelperLineVertices.size()));
		if (!m_HelperLineVertices.empty())
		{
			m_HelperLineVBO->SetData(
				m_HelperLineVertices.data(),
				static_cast<uint32_t>(m_HelperLineVertices.size() * sizeof(glm::vec3)),
				0);
		}

		ReCreateHandlePointBuffer(static_cast<uint32_t>(m_HandlePointVertices.size()));
		if (!m_HandlePointVertices.empty())
		{
			m_HandlePointVBO->SetData(
				m_HandlePointVertices.data(),
				static_cast<uint32_t>(m_HandlePointVertices.size() * sizeof(EditorHandlePointData)),
				0);
		}

		m_HelperDirty = false;
	}

	void LineRenderer::BuildSelectedHelperGeometry()
	{
		m_HelperLineVertices.clear();
		m_HandlePointVertices.clear();

		if (m_CurveType != CurveType::BezierCubic || m_SelectedAnchorIndex == -1)
			return;

		const int anchorIndex = m_SelectedAnchorIndex;
		if (!IsAnchorPoint(anchorIndex))
			return;

		const auto& anchor = m_ControlPoints[anchorIndex];
		const int leftHandle = GetLeftHandleIndexForAnchor(anchorIndex);
		const int rightHandle = GetRightHandleIndexForAnchor(anchorIndex);

		const auto appendDashedSegment = [&](const glm::vec3& start, const glm::vec3& end)
		{
			const glm::vec3 direction = end - start;
			const float length = glm::length(direction);
			if (length < 0.0001f)
				return;

			const glm::vec3 normalized = direction / length;
			const float dashLength = 0.16f;
			const float gapLength = 0.08f;

			float traveled = 0.0f;
			while (traveled < length)
			{
				const float dashStart = traveled;
				const float dashEnd = std::min(traveled + dashLength, length);
				m_HelperLineVertices.push_back(start + normalized * dashStart);
				m_HelperLineVertices.push_back(start + normalized * dashEnd);
				traveled += dashLength + gapLength;
			}
		};

		const auto appendHandlePoint = [&](int handleIndex)
		{
			if (handleIndex < 0 || handleIndex >= static_cast<int>(m_ControlPoints.size()))
				return;

			EditorHandlePointData point{};
			point.position = m_ControlPoints[handleIndex].position;
			point.color = m_ControlPoints[handleIndex].color;
			point.radius = GetControlPointRadius(handleIndex) + 3.0f;
			point.index = handleIndex;
			m_HandlePointVertices.push_back(point);
		};

		if (leftHandle != -1)
		{
			appendDashedSegment(anchor.position, m_ControlPoints[leftHandle].position);
			appendHandlePoint(leftHandle);
		}

		if (rightHandle != -1)
		{
			appendDashedSegment(anchor.position, m_ControlPoints[rightHandle].position);
			appendHandlePoint(rightHandle);
		}
	}

	void LineRenderer::ValidateSelectedControlPoint()
	{
		if (m_SelectedControlPointIndex < 0 || m_SelectedControlPointIndex >= static_cast<int>(m_ControlPoints.size()))
		{
			m_SelectedControlPointIndex = -1;
			m_SelectedAnchorIndex = -1;
			return;
		}

		m_SelectedAnchorIndex = GetAnchorIndexForControlPoint(m_SelectedControlPointIndex);
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

	void LineRenderer::ApplyHandleMode(int movedHandleIndex)
	{
		if (movedHandleIndex < 0 || movedHandleIndex >= (int)m_ControlPoints.size())
			return;

		const int anchorIndex = GetAnchorIndexForControlPoint(movedHandleIndex);
		if (anchorIndex == -1 || IsAnchorPoint(movedHandleIndex))
			return;

		const BezierHandleMode mode = GetHandleModeForAnchor(anchorIndex);
		if (mode == BezierHandleMode::Free)
			return;

		int oppositeHandle = -1;
		if (movedHandleIndex % 3 == 2)
		{
			oppositeHandle = anchorIndex + 1;
		}
		else if (movedHandleIndex % 3 == 1)
		{
			oppositeHandle = anchorIndex - 1;
		}

		if (oppositeHandle < 0 || oppositeHandle >= (int)m_ControlPoints.size())
			return;

		const glm::vec3 anchor = m_ControlPoints[anchorIndex].position;
		const glm::vec3 movedDirection = m_ControlPoints[movedHandleIndex].position - anchor;
		if (glm::dot(movedDirection, movedDirection) < 0.000001f)
			return;

		if (mode == BezierHandleMode::Mirrored)
		{
			m_ControlPoints[oppositeHandle].position = anchor - movedDirection;
			return;
		}

		glm::vec3 oppositeDirection = m_ControlPoints[oppositeHandle].position - anchor;
		float oppositeLength = glm::length(oppositeDirection);
		if (oppositeLength < 0.0001f)
		{
			oppositeLength = glm::length(movedDirection);
		}

		const glm::vec3 normalized = glm::normalize(movedDirection);
		m_ControlPoints[oppositeHandle].position = anchor - normalized * oppositeLength;
	}

	void LineRenderer::ResizeHandleModesToMatchAnchors()
	{
		const size_t anchorCount = m_ControlPoints.empty() ? 0 : (m_ControlPoints.size() + 2) / 3;
		m_HandleModes.resize(anchorCount, BezierHandleMode::Free);
	}

	BezierHandleMode LineRenderer::GetHandleModeForAnchor(int anchorIndex) const
	{
		if (!IsAnchorPoint(anchorIndex))
			return BezierHandleMode::Free;

		const size_t anchorSlot = static_cast<size_t>(anchorIndex / 3);
		if (anchorSlot >= m_HandleModes.size())
			return BezierHandleMode::Free;

		return m_HandleModes[anchorSlot];
	}

	void LineRenderer::SetHandleModeForAnchor(int anchorIndex, BezierHandleMode mode)
	{
		if (!IsAnchorPoint(anchorIndex))
			return;

		ResizeHandleModesToMatchAnchors();
		m_HandleModes[static_cast<size_t>(anchorIndex / 3)] = mode;
		m_Dirty = true;
		m_HelperDirty = true;
	}

}
