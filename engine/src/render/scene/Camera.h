#pragma once

#include <glm/glm.hpp>
namespace Kita {


	enum class ProjectionMode
	{
		Orthographic = 0,
		Perspective
	};


	class Camera
	{
	public:
		
		virtual ~Camera() {}
		
		virtual const glm::mat4& GetProjectionMatrix() const { return  m_ProjectionMatrix; }
		virtual const glm::mat4& GetViewMatrix() const { return  m_ViewMatrix; }
		virtual const glm::mat4& GetVPMatrix() const { return  m_ViewMatrix * m_ProjectionMatrix; }

		virtual void  CaculateProjectionMatrix() = 0;
		virtual void  CaculateViewMatrix() = 0;
		virtual void  SetAspectRatio(float ratio) = 0;

	protected:

		ProjectionMode m_ProjectionMode;
		glm::mat4  m_ViewMatrix, m_ProjectionMatrix;
	};

}