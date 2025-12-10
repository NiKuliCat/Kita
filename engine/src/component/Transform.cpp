#include "kita_pch.h"
#include "Transform.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
namespace Kita {

	glm::mat4& Transform::GetTransformMatrix()
	{
		glm::mat4 translate = glm::translate(glm::mat4(1.0f), m_Position);
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_Scale);
		
		glm::mat4 rotate = glm::toMat4(  glm::quat(glm::radians(m_Rotation)));
		return  translate * rotate * scale;
	}

	glm::mat4& Transform::GetViewMatrix()
	{
		glm::mat4 translate = glm::translate(glm::mat4(1.0f), m_Position);
		glm::mat4 rotate = glm::toMat4(glm::quat(glm::radians(m_Rotation)));
		return glm::inverse(translate * rotate);
	}
}