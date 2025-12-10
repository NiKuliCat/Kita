#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
namespace Kita {

	class Transform
	{
	public:
		Transform() = default;
		Transform(const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
			: m_Position(position), m_Rotation(rotation), m_Scale(scale) {}

		glm::vec3& GetPosition() { return m_Position; }
		const glm::vec3& GetPosition() const { return m_Position; }

		glm::vec3& GetRotation() { return m_Rotation; }
		const glm::vec3& GetRotation() const { return m_Rotation; }

		glm::vec3& GetScale() { return m_Scale; }
		const glm::vec3& GetScale() const { return m_Scale; }

		void SetRotation(const glm::vec3& rotation) { m_Rotation = rotation; }
		void SetPosition(const glm::vec3& position) { m_Position = position; }	
		void SetScale(const glm::vec3& scale) { m_Scale = scale; }

		glm::mat4& GetTransformMatrix();
		glm::mat4& GetViewMatrix();
		glm::vec3& GetFrontDir();

	private:
		glm::vec3 m_Position = {0.0,0.0,0.0 };
		glm::vec3 m_Rotation = { 0.0,0.0,0.0 };
		glm::vec3 m_Scale = {1.0,1.0,1.0};
	};
}