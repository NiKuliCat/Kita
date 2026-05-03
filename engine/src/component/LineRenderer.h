#pragma once
#include <algorithm>
#include <vector>
#include <glm/glm.hpp>
#include "render/Buffer.h"
#include "render/VertexArray.h"
namespace Kita {

	enum class CurveType
	{
		Polyline = 0,
		BezierCubic
	};

	enum class BezierHandleMode
	{
		Free = 0,
		Aligned,
		Mirrored
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

		std::vector<PointData>& GetControlPoints() { return m_ControlPoints; }
		const std::vector<PointData>& GetControlPoints() const { return m_ControlPoints; }

		const PointData& GetControlPointByIndex(const int index) const;
		PointData& GetControlPointByIndex(const int index);

		void SetControlPointColorByIndex(const glm::vec4& color, const int index);

		void SetCurveType(CurveType type) { m_CurveType = type; m_Dirty = true; m_HelperDirty = true; }
		CurveType GetCurveType() const { return m_CurveType; }

		void SetLineWidth(float width) { m_LineWidth = std::max(1.0f, width); }
		float GetLineWidth() const { return m_LineWidth; }

		void SetLineColor(const glm::vec4& color) { m_LineColor = color; }
		const glm::vec4& GetLineColor() const { return m_LineColor; }
		uint32_t GetControlPointCount() const { return static_cast<uint32_t>(m_ControlPoints.size()); }

		bool IsAnchorControlPoint(int index) const;

		BezierHandleMode GetHandleModeForPoint(int index) const;
		void SetHandleModeForPoint(int index, BezierHandleMode mode);

		void AppendBezierSegment();
		void RemoveLastBezierSegment();

		float GetControlPointRadius(int index) const;
		void ResetControlPointVisual(int index);

		const Ref<VertexArray>& GetCurveVAO() const { return m_Curve_VAO; }
		uint32_t GetCurveVertexCount() const { return m_CurveVertexCount; }

		void RebuildIfNeeded();

		void MoveControlPoint(int index, const glm::vec3& newPosition);
		void SetSelectedControlPoint(int index);
		void ClearSelectedControlPoint();
		void RenderEditorHelpers(const glm::mat4& model, uint32_t objectId);

	private:
		int GetAnchorIndexForControlPoint(int index) const;
		int GetLeftHandleIndexForAnchor(int anchorIndex) const;
		int GetRightHandleIndexForAnchor(int anchorIndex) const;
		glm::vec4 GetDefaultControlPointColor(int index) const;
		void ResetAllControlPointVisuals();

		glm::vec3 EvaluateBezierCubic(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t);
		void BuildBezierCubic();
		void InitBuffer();
		void ReCreateBuffer(const uint32_t size);
		void InitEditorHelperBuffers();
		void ReCreateHelperLineBuffer(uint32_t vertexCount);
		void ReCreateHandlePointBuffer(uint32_t pointCount);
		void RebuildEditorHelpersIfNeeded();
		void BuildSelectedHelperGeometry();
		void ValidateSelectedControlPoint();

		bool IsAnchorPoint(int index) const;
		void MoveAnchorWithHandles(int anchorIndex, const glm::vec3& delta);
		void ApplyHandleMode(int movedHandleIndex);
		void ResizeHandleModesToMatchAnchors();
		BezierHandleMode GetHandleModeForAnchor(int anchorIndex) const;
		void SetHandleModeForAnchor(int anchorIndex, BezierHandleMode mode);


	private:
		struct EditorHandlePointData
		{
			glm::vec3 position;
			glm::vec4 color;
			float radius;
			int index;
		};

		std::vector<PointData> m_ControlPoints;
		std::vector<glm::vec3> m_CurveVertices;
		std::vector<glm::vec3> m_HelperLineVertices;
		std::vector<EditorHandlePointData> m_HandlePointVertices;

		CurveType m_CurveType = CurveType::BezierCubic;

		Ref<VertexArray> m_Curve_VAO = nullptr;
		Ref<VertexBuffer> m_Curve_VBO = nullptr;
		BufferLayout m_CurveVertexLayout;
		Ref<VertexArray> m_HelperLineVAO = nullptr;
		Ref<VertexBuffer> m_HelperLineVBO = nullptr;
		BufferLayout m_HelperLineLayout;
		Ref<VertexArray> m_HandlePointVAO = nullptr;
		Ref<VertexBuffer> m_HandlePointVBO = nullptr;
		BufferLayout m_HandlePointLayout;

		int m_SegmentCountPerBezier = 128;
		float m_LineWidth = 1.0f;
		glm::vec4 m_LineColor = glm::vec4(1.0f);
		std::vector<BezierHandleMode> m_HandleModes;

		bool m_Dirty = true;
		bool m_HelperDirty = true;

		uint32_t m_CurveVertexCapacity = 0;
		uint32_t m_CurveVertexCount = 0;
		uint32_t m_HelperLineCapacity = 0;
		uint32_t m_HandlePointCapacityBytes = 0;
		int m_SelectedControlPointIndex = -1;
		int m_SelectedAnchorIndex = -1;
	};
}
