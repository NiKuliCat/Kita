#include "kita_pch.h"
#include "OrthographicCamera.h"
#include <glm/gtc/matrix_transform.hpp>
namespace Kita{



	OrthographicCamera::OrthographicCamera(const float orthoSize, const float aspect, const float nearPlane, const float farPlane)
		:m_OrthoSize(orthoSize),m_Aspect(aspect),m_Near(nearPlane),m_Far(farPlane)
	{
		CaculateProjectionMatrix();
	}

	OrthographicCamera::~OrthographicCamera()
	{
		
	}

	void OrthographicCamera::CaculateProjectionMatrix()
	{
		float width = m_Aspect * m_OrthoSize;
		float height = m_OrthoSize;

		m_ProjectionMatrix = glm::ortho(-width, width, -height, height, m_Near, m_Far);
	}

	void OrthographicCamera::CaculateViewMatrix()
	{
	}

}