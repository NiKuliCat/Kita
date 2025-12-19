#include "renderer_pch.h"
#include "ViewportCamera.h"
#include <core/Input.h>
#include <glfw/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
namespace Kita {

	ViewportCamera::ViewportCamera(const float fov, const float aspect, const float nearPlane, const float farPlane)
		:m_FOV(fov), m_Aspect(aspect), m_Near(nearPlane), m_Far(farPlane)
	{
		UpdateViewMatrix();
		m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_Aspect, m_Near, m_Far);
	}


	void ViewportCamera::OnUpdate(float deltaTime)
	{
		if (Input::IsKeyPressed(Key::LeftAlt))
		{
			const glm::vec2 mouse{ Input::GetMouseX(), Input::GetMouseY() };
			glm::vec2 delta = (mouse - m_MousePosition) * 0.003f;

			m_MousePosition = mouse;


			if (Input::IsMouseButtonPressed(Mouse::ButtonMiddle))
				MousePan(delta);
			else if (Input::IsMouseButtonPressed(Mouse::ButtonLeft))
				MouseRotate(delta);
			else if (Input::IsMouseButtonPressed(Mouse::ButtonRight))
				MouseZoom(delta.y);

			UpdateViewMatrix();
		}


	}
	void ViewportCamera::OnEvent(Event& event)
	{
		EventDisPatcher dispatcher(event);
		dispatcher.Dispatcher<MouseScrolledEvent>(BIND_EVENT_FUNC(ViewportCamera::OnMouseScroll));
	}

	glm::vec3& ViewportCamera::GetUpDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3& ViewportCamera::GetForwardDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f)); //  为什么不是1.0f
	}

	glm::vec3& ViewportCamera::GetRightDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::quat ViewportCamera::GetOrientation() const
	{
		return glm::quat(glm::vec3(-m_Pitch, -m_Yaw, 0.0f));
	}

	void ViewportCamera::UpdateProjectionMatrix()
	{
		m_Aspect = m_ViewportWidth / m_ViewportHeight;
		m_ProjectionMatrix = glm::perspective(glm::radians(m_FOV), m_Aspect, m_Near, m_Far);
	}

	void ViewportCamera::UpdateViewMatrix()
	{
		m_Position = CalculatePosition();
		glm::quat orientation = GetOrientation();
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position) * glm::toMat4(orientation);
		m_ViewMatrix = glm::inverse(transform);
	}

	bool ViewportCamera::OnMouseScroll(MouseScrolledEvent& event)
	{
		float delta = event.GetOffsetY() * 0.1f;
		MouseZoom(delta);
		UpdateViewMatrix();
		return false;
	}

	void ViewportCamera::MousePan(const glm::vec2& delta)
	{
		auto [xSpeed, ySpeed] = PanSpeed();
		m_FocusePosition += -GetRightDirection() * delta.x * xSpeed * m_Distance;
		m_FocusePosition += GetUpDirection() * delta.y * ySpeed * m_Distance;
	}

	void ViewportCamera::MouseRotate(const glm::vec2& delta)
	{
		float yawSign = GetUpDirection().y < 0 ? -1.0f : 1.0f;
		m_Yaw += yawSign * delta.x * RotationSpeed();
		m_Pitch += delta.y * RotationSpeed();
	}

	void ViewportCamera::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();
		if (m_Distance < 1.0f)
		{
			m_FocusePosition += GetForwardDirection();
			m_Distance = 1.0f;
		}
	}

	glm::vec3 ViewportCamera::CalculatePosition() const
	{
		return m_FocusePosition - GetForwardDirection() * m_Distance;
	}

	std::pair<float, float> ViewportCamera::PanSpeed() const
	{
		float x = std::min(m_ViewportWidth / 1000.0f, 2.4f); // max = 2.4f
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min(m_ViewportHeight / 1000.0f, 2.4f); // max = 2.4f
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float ViewportCamera::RotationSpeed() const
	{
		return 0.8f;
	}

	float ViewportCamera::ZoomSpeed() const
	{
		float distance = m_Distance * 0.2f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		speed = std::min(speed, 100.0f); // max speed = 100
		return speed;
	}



}