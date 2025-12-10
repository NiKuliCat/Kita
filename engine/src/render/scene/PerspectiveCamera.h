#pragma once
#include "Camera.h"



namespace Kita {


	class PerspectiveCamera : public Camera
	{
	public:
		PerspectiveCamera() = default;
		PerspectiveCamera(const float fov, const float aspect, const float nearPlane, const float farPlane);
		~PerspectiveCamera() {}


	public:

		virtual void CaculateProjectionMatrix() override;
		virtual void SetAspectRatio(float ratio) override {
			m_Aspect = ratio; CaculateProjectionMatrix();
		}
	private:
		float m_FOV;
		float m_Aspect;
		float m_Near, m_Far;
	};
}