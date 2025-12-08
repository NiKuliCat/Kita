#pragma once

#include "Camera.h"


namespace Kita {


	class OrthographicCamera : public Camera
	{
	public:
		OrthographicCamera() = default;
		OrthographicCamera(const float orthoSize, const float aspect, const float nearPlane, const float farPlane);
		~OrthographicCamera();


	public:

		virtual void  CaculateProjectionMatrix() override;
		virtual void  CaculateViewMatrix() override;


	private:
		float m_OrthoSize;
		float m_Aspect;
		float m_Near, m_Far;

	};

}