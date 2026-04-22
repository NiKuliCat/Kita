#include "kita_pch.h"
#include "Transform.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/matrix_decompose.hpp>
namespace Kita {

	glm::mat4 Transform::GetTransformMatrix() const
	{
		glm::mat4 translate = glm::translate(glm::mat4(1.0f), m_Position);
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), m_Scale);
		
		glm::mat4 rotate = glm::toMat4(  glm::quat(glm::radians(m_Rotation)));
		return  translate * rotate * scale;
	}

	glm::mat4 Transform::GetViewMatrix() const
	{
		glm::mat4 translate = glm::translate(glm::mat4(1.0f), m_Position);
		glm::mat4 rotate = glm::toMat4(glm::quat(glm::radians(m_Rotation)));
		return glm::inverse(translate * rotate);
	}
	glm::vec3 Transform::GetFrontDir() const
	{
		glm::quat q = glm::quat(glm::radians(m_Rotation));
		glm::vec3  front = glm::vec3(0.0, 0.0, -1.0f);

		return glm::normalize(q * front);
	}
	bool Transform::DecomposeTransformMatrix(const glm::mat4& transform, glm::vec3& out_Translation, glm::vec3& out_Rotation, glm::vec3& out_Scale)
	{
		using namespace glm;
		using T = float;

		mat4 LocalMatrix(transform);

		// Normalize the matrix.
		if (epsilonEqual(LocalMatrix[3][3], static_cast<float>(0), epsilon<T>()))
			return false;

		// First, isolate perspective.  This is the messiest.
		if (
			epsilonNotEqual(LocalMatrix[0][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[1][3], static_cast<T>(0), epsilon<T>()) ||
			epsilonNotEqual(LocalMatrix[2][3], static_cast<T>(0), epsilon<T>()))
		{
			// Clear the perspective partition
			LocalMatrix[0][3] = LocalMatrix[1][3] = LocalMatrix[2][3] = static_cast<T>(0);
			LocalMatrix[3][3] = static_cast<T>(1);
		}

		// Next take care of translation (easy).
		out_Translation = vec3(LocalMatrix[3]);
		LocalMatrix[3] = vec4(0, 0, 0, LocalMatrix[3].w);

		vec3 Row[3], Pdum3;

		// Now get scale and shear.
		for (length_t i = 0; i < 3; ++i)
			for (length_t j = 0; j < 3; ++j)
				Row[i][j] = LocalMatrix[i][j];

		// Compute X scale factor and normalize first row.
		out_Scale.x = length(Row[0]);
		Row[0] = detail::scale(Row[0], static_cast<T>(1));
		out_Scale.y = length(Row[1]);
		Row[1] = detail::scale(Row[1], static_cast<T>(1));
		out_Scale.z = length(Row[2]);
		Row[2] = detail::scale(Row[2], static_cast<T>(1));

		// At this point, the matrix (in rows[]) is orthonormal.
		// Check for a coordinate system flip.  If the determinant
		// is -1, then negate the matrix and the scaling factors.
#if 0
		Pdum3 = cross(Row[1], Row[2]); // v3Cross(row[1], row[2], Pdum3);
		if (dot(Row[0], Pdum3) < 0)
		{
			for (length_t i = 0; i < 3; i++)
			{
				scale[i] *= static_cast<T>(-1);
				Row[i] *= static_cast<T>(-1);
			}
		}
#endif

		out_Rotation.y = asin(-Row[0][2]);
		if (cos(out_Rotation.y) != 0) {
			out_Rotation.x = atan2(Row[1][2], Row[2][2]);
			out_Rotation.z = atan2(Row[0][1], Row[0][0]);
		}
		else {
			out_Rotation.x = atan2(-Row[2][0], Row[1][1]);
			out_Rotation.z = 0;
		}

		/*out_Rotation.x = glm::degrees(out_Rotation.x);
		out_Rotation.y = glm::degrees(out_Rotation.y);
		out_Rotation.z = glm::degrees(out_Rotation.z);*/

		return true;
	}
}
