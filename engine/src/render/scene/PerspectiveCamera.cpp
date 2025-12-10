#include "kita_pch.h"
#include "PerspectiveCamera.h"
#include <glm/gtc/matrix_transform.hpp>


namespace Kita {

	PerspectiveCamera::PerspectiveCamera(const float fov, const float aspect, const float nearPlane, const float farPlane)
		:m_FOV(fov), m_Aspect(aspect), m_Near(nearPlane), m_Far(farPlane)
	{
		CaculateProjectionMatrix();
	}

	void PerspectiveCamera::CaculateProjectionMatrix()
	{
		m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_Aspect, m_Near, m_Far);
	}

}