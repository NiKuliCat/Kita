#pragma once
#include "render/Renderer.h"
#include <cstddef>

namespace Kita {


	struct GizmoPointUBOData
	{
		glm::vec3 positionWS;
		glm::vec4 color;
		float radius;
		int id = -1;
	};

	static_assert(sizeof(GizmoPointUBOData) == 36, "GizmoPointUBOData layout must match BufferLayout stride (36 bytes).");
	static_assert(offsetof(GizmoPointUBOData, positionWS) == 0, "positionWS offset mismatch.");
	static_assert(offsetof(GizmoPointUBOData, color) == 12, "color offset mismatch.");
	static_assert(offsetof(GizmoPointUBOData, radius) == 28, "radius offset mismatch.");
	static_assert(offsetof(GizmoPointUBOData, id) == 32, "id offset mismatch.");

	

	class Gizmo
	{
	public:
		struct SceneGizmoData
		{

			BufferLayout m_PointLayout = {
				{ShaderDataType::Float3,"positionWS"},
				{ShaderDataType::Float4,"color"},
				{ShaderDataType::Float,"radius"},
				{ShaderDataType::Int,"id"}
			};

			std::vector<GizmoPointUBOData> m_PointsData;

			Ref<VertexArray> m_Points_VAO = nullptr;
			Ref<VertexBuffer> m_Points_VBO = nullptr;
			uint32_t m_PointCapacityBytes = 0;
		};

		static void Init();
		static void DrawPoints(const std::vector<GizmoPointUBOData>& points);
		static void FlushAllPoints();


	private:
		static SceneGizmoData* m_SceneGizmoData;

	};
}
