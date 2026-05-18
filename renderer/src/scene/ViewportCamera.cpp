#include "renderer_pch.h"
#include "ViewportCamera.h"
#include <core/Input.h>
#include <glfw/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Kita {

	namespace
	{
		constexpr float kMaxPitch = glm::radians(89.0f);
	}

	ViewportCamera::ViewportCamera()
		: ViewportCamera(45.0f, 16.0f / 9.0f, 0.1f, 1000.0f)
	{
	}

	ViewportCamera::ViewportCamera(const float fov, const float aspect, const float nearPlane, const float farPlane)
		:m_FOV(fov), m_Aspect(aspect), m_Near(nearPlane), m_Far(farPlane)
	{
		m_ProjectionMatrix = glm::perspectiveRH_ZO(glm::radians(m_FOV), m_Aspect, m_Near, m_Far);
		m_ProjectionMatrix[1][1] *= -1.0f;
		m_GizmoProjectionMatrix = m_ProjectionMatrix;
		m_GizmoProjectionMatrix[1][1] *= -1.0f;
		m_Position = CalculatePosition();
		UpdateViewMatrix();
	}


	void ViewportCamera::OnUpdate(float deltaTime)
	{
		const glm::vec2 mouse{ Input::GetMouseX(), Input::GetMouseY() };
		glm::vec2 delta = (mouse - m_MousePosition) * 0.003f;
		m_MousePosition = mouse;

		const bool altDown = Input::IsKeyPressed(Key::LeftAlt) || Input::IsKeyPressed(Key::RightAlt);
		const bool rightMouseDown = Input::IsMouseButtonPressed(Mouse::ButtonRight);
		const bool middleMouseDown = Input::IsMouseButtonPressed(Mouse::ButtonMiddle);
		const bool leftMouseDown = Input::IsMouseButtonPressed(Mouse::ButtonLeft);

		if (rightMouseDown && !altDown)
		{
			FlightRotate(delta);
			FlightMove(deltaTime);
			UpdateViewMatrix();
		}
		else if (altDown)
		{
			if (leftMouseDown)
				MouseRotate(delta);
			else if (middleMouseDown)
				MousePan(delta);
			else if (rightMouseDown)
				MouseZoom(delta.y);

			UpdateViewMatrix();
		}
		else if (middleMouseDown)
		{
			MousePan(delta);
			UpdateViewMatrix();
		}


	}
	void ViewportCamera::OnEvent(Event& event)
	{
		EventDisPatcher dispatcher(event);
		dispatcher.Dispatcher<MouseScrolledEvent>(BIND_EVENT_FUNC(ViewportCamera::OnMouseScroll));
	}

	void ViewportCamera::SetFocusTarget(const glm::vec3& target)
	{
		m_FocusePosition = target;
		m_Position = CalculatePosition();
		UpdateViewMatrix();
	}

	void ViewportCamera::FocusOnPoint(const glm::vec3& target, float radius)
	{
		m_FocusePosition = target;

		const float safeRadius = std::max(radius, 0.5f);
		const float framedDistance = std::max(safeRadius * 2.5f, 2.0f);
		m_Distance = framedDistance;

		m_Position = CalculatePosition();
		UpdateViewMatrix();
	}

	const glm::mat4& ViewportCamera::GetGizmoProjection() const
	{
		return m_GizmoProjectionMatrix;
	}

	glm::vec3 ViewportCamera::GetUpDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 1.0f, 0.0f));
	}

	glm::vec3 ViewportCamera::GetForwardDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(0.0f, 0.0f, -1.0f));
	}

	glm::vec3 ViewportCamera::GetRightDirection() const
	{
		return glm::rotate(GetOrientation(), glm::vec3(1.0f, 0.0f, 0.0f));
	}

	glm::quat ViewportCamera::GetOrientation() const
	{
		const glm::quat yawRotation = glm::angleAxis(-m_Yaw, glm::vec3(0.0f, 1.0f, 0.0f));
		const glm::quat pitchRotation = glm::angleAxis(-m_Pitch, glm::vec3(1.0f, 0.0f, 0.0f));
		return glm::normalize(yawRotation * pitchRotation);
	}

	void ViewportCamera::UpdateProjectionMatrix()
	{
		m_Aspect = m_ViewportWidth / m_ViewportHeight;
		m_ProjectionMatrix = glm::perspectiveRH_ZO(glm::radians(m_FOV), m_Aspect, m_Near, m_Far);
		m_ProjectionMatrix[1][1] *= -1.0f;
		m_GizmoProjectionMatrix = m_ProjectionMatrix;
		m_GizmoProjectionMatrix[1][1] *= -1.0f;
	}

	void ViewportCamera::UpdateViewMatrix()
	{
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
		m_Position = CalculatePosition();
	}

	void ViewportCamera::MouseRotate(const glm::vec2& delta)
	{
		m_Yaw += delta.x * RotationSpeed();
		m_Pitch = glm::clamp(m_Pitch - delta.y * RotationSpeed(), -kMaxPitch, kMaxPitch);
		m_Position = CalculatePosition();
	}

	void ViewportCamera::MouseZoom(float delta)
	{
		m_Distance -= delta * ZoomSpeed();
		if (m_Distance < 1.0f)
		{
			m_FocusePosition += GetForwardDirection();
			m_Distance = 1.0f;
		}

		m_Position = CalculatePosition();
	}

	void ViewportCamera::FlightRotate(const glm::vec2& delta)
	{
		m_Yaw += delta.x * RotationSpeed();
		m_Pitch = glm::clamp(m_Pitch - delta.y * RotationSpeed(), -kMaxPitch, kMaxPitch);
		m_FocusePosition = m_Position + GetForwardDirection() * m_Distance;
	}

	void ViewportCamera::FlightMove(float deltaTime)
	{
		glm::vec3 velocity(0.0f);
		const float speed = FlightSpeed() * deltaTime;

		if (Input::IsKeyPressed(Key::W))
			velocity += GetForwardDirection();
		if (Input::IsKeyPressed(Key::S))
			velocity -= GetForwardDirection();
		if (Input::IsKeyPressed(Key::D))
			velocity += GetRightDirection();
		if (Input::IsKeyPressed(Key::A))
			velocity -= GetRightDirection();
		if (Input::IsKeyPressed(Key::Q))
			velocity += glm::vec3(0.0f, 1.0f, 0.0f);
		if (Input::IsKeyPressed(Key::E))
			velocity -= glm::vec3(0.0f, 1.0f, 0.0f);

		if (glm::dot(velocity, velocity) > 0.0f)
			velocity = glm::normalize(velocity);

		m_Position += velocity * speed;
		m_FocusePosition = m_Position + GetForwardDirection() * m_Distance;
	}

	glm::vec3 ViewportCamera::CalculatePosition() const
	{
		return m_FocusePosition - GetForwardDirection() * m_Distance;
	}

	std::pair<float, float> ViewportCamera::PanSpeed() const
	{
		float x = std::min(m_ViewportWidth / 1000.0f, 2.4f);
		float xFactor = 0.0366f * (x * x) - 0.1778f * x + 0.3021f;

		float y = std::min(m_ViewportHeight / 1000.0f, 2.4f);
		float yFactor = 0.0366f * (y * y) - 0.1778f * y + 0.3021f;

		return { xFactor, yFactor };
	}

	float ViewportCamera::RotationSpeed() const
	{
		return m_RotationSpeedValue;
	}

	float ViewportCamera::ZoomSpeed() const
	{
		float distance = m_Distance * 0.2f;
		distance = std::max(distance, 0.0f);
		float speed = distance * distance;
		speed = std::min(speed, 100.0f);
		return speed * m_ZoomSpeedScale;
	}

	float ViewportCamera::FlightSpeed() const
	{
		float speed = std::max(m_Distance, 1.0f) * 0.06f * m_FlightSpeedScale;
		if (Input::IsKeyPressed(Key::LeftShift) || Input::IsKeyPressed(Key::RightShift))
			speed *= 2.5f;
		return speed;
	}

}
