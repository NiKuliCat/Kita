#include "kita_pch.h"
#include "Gizmo.h"
#include "core/Log.h"
namespace Kita {

	Gizmo::SceneGizmoData* Gizmo::m_SceneGizmoData = nullptr;


	void Gizmo::Init()
	{
		
	}

	void Gizmo::DrawPoints(const std::vector<GizmoPointUBOData>& points)
	{
		m_SceneGizmoData->m_PointsData.insert(m_SceneGizmoData->m_PointsData.end(),points.begin(), points.end());
	}

	void Gizmo::FlushAllPoints(const glm::mat4& model,const uint32_t id)
	{
		
	}

}
