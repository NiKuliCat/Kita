#include "kita_pch.h"
#include "Gizmo.h"
#include "core/Log.h"
#include "render/ShaderLibrary.h"
namespace Kita {

	Gizmo::SceneGizmoData* Gizmo::m_SceneGizmoData = nullptr;


	void Gizmo::Init()
	{
		if (!m_SceneGizmoData) m_SceneGizmoData = new SceneGizmoData();

		m_SceneGizmoData->m_PointsData = {};
		m_SceneGizmoData->m_Points_VAO = VertexArray::Create();

		m_SceneGizmoData->m_PointCapacityBytes = 1024 * sizeof(GizmoPointUBOData);

		m_SceneGizmoData->m_Points_VBO = VertexBuffer::Create(m_SceneGizmoData->m_PointCapacityBytes);

		auto layout = m_SceneGizmoData->m_PointLayout;
		m_SceneGizmoData->m_Points_VBO->SetLayout(layout);
		m_SceneGizmoData->m_Points_VAO->AddVertexBuffer(m_SceneGizmoData->m_Points_VBO);
	}

	void Gizmo::DrawPoints(const std::vector<GizmoPointUBOData>& points)
	{
		m_SceneGizmoData->m_PointsData.insert(m_SceneGizmoData->m_PointsData.end(),points.begin(), points.end());
	}

	void Gizmo::FlushAllPoints(const glm::mat4& model,const uint32_t id)
	{
		KITA_CORE_ASSERT(m_SceneGizmoData, "Gizmo::Init must be called before FlushAllPoints.");

		auto& data = m_SceneGizmoData->m_PointsData;
		if (data.empty()) return;

		const uint32_t bytes = (uint32_t)(data.size() * sizeof(GizmoPointUBOData));

		if (bytes > m_SceneGizmoData->m_PointCapacityBytes)
		{
			while (m_SceneGizmoData->m_PointCapacityBytes < bytes)
				m_SceneGizmoData->m_PointCapacityBytes *= 2;

			m_SceneGizmoData->m_Points_VBO = VertexBuffer::Create(m_SceneGizmoData->m_PointCapacityBytes);
			auto layout = m_SceneGizmoData->m_PointLayout;
			m_SceneGizmoData->m_Points_VBO->SetLayout(layout);

			m_SceneGizmoData->m_Points_VAO = VertexArray::Create();
			m_SceneGizmoData->m_Points_VAO->AddVertexBuffer(m_SceneGizmoData->m_Points_VBO);
		}

		m_SceneGizmoData->m_Points_VBO->SetData(data.data(), bytes);

		auto shader = ShaderLibrary::GetInstance().Get("GizmoPoint");
		shader->SetMat4("Matrix_M", model);
		shader->SetInt("id", id);
		Renderer::DrawGizmoPoints(m_SceneGizmoData->m_Points_VAO, shader, static_cast<uint32_t>(data.size()));

		data.clear();
	}

}
