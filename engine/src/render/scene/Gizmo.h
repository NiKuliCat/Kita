#pragma once
#include "render/Renderer.h"
#include <cstddef>

namespace Kita {


	struct GizmoPointUBOData
	{
		glm::vec3 position;
		glm::vec4 color;
		float radius;
	};


	

	class Gizmo
	{
	public:
		struct SceneGizmoData
		{

			BufferLayout m_PointLayout = {
				{ShaderDataType::Float3,"position"},
				{ShaderDataType::Float4,"color"},
				{ShaderDataType::Float,"radius"}
			};

			std::vector<GizmoPointUBOData> m_PointsData;

			Ref<VertexArray> m_Points_VAO = nullptr;
			Ref<VertexBuffer> m_Points_VBO = nullptr;
			uint32_t m_PointCapacityBytes = 0;
		};

		static void Init();
		static void DrawPoints(const std::vector<GizmoPointUBOData>& points);
		static void FlushAllPoints(const glm::mat4& model,const uint32_t id);


	private:
		static SceneGizmoData* m_SceneGizmoData;

	};
}
