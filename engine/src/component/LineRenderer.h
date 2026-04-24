#pragma once
#include <glm/glm.hpp>
#include "render/Buffer.h"
#include "render/VertexArray.h"
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

		void SetLineWidth(float width) { m_LineWidth = std::max(1.0f, width); }
		float GetLineWidth() const { return m_LineWidth; }

		void SetLineColor(const glm::vec4& color) { m_LineColor = color; }
		const glm::vec4& GetLineColor() const { return m_LineColor; }

		const Ref<VertexArray>& GetCurveVAO() const { return m_Curve_VAO; }
		uint32_t GetCurveVertexCount() const { return m_CurveVertexCount; }

		void RebuildIfNeeded();
		void MarkDirty() { m_Dirty = true; }

	private:
		glm::vec3 EvaluateBezierCubic(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t);
		void BuildBezierCubic();
		void InitBuffer();
		void ReCreateBuffer(const uint32_t size);




	private:

		std::vector<PointData> m_ControlPoints;
		std::vector<glm::vec3> m_CurveVertices;

		CurveType m_CurveType = CurveType::BezierCubic;

		Ref<VertexArray> m_Curve_VAO = nullptr;
		Ref<VertexBuffer> m_Curve_VBO = nullptr;
		BufferLayout m_CurveVertexLayout;

		int m_SegmentCountPerBezier = 128;
		float m_LineWidth = 1.0f;
		glm::vec4 m_LineColor = glm::vec4(1.0f);

		bool m_Dirty = true;

		uint32_t m_CurveVertexCapacity = 0;
		uint32_t m_CurveVertexCount = 0;
	};
}
