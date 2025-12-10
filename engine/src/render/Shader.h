#pragma once
#include "core/Core.h"
#include <glm/glm.hpp>
namespace Kita {


	enum class ShaderType
	{
		Vertex,
		Fragment
	};

	class Shader
	{
	public:

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		virtual const uint32_t GetID() const = 0;
		virtual const std::string& GetName() const = 0;

		virtual void SetInt(const std::string& name, const uint32_t value) = 0;
		virtual void SetMat4(const std::string& name, const glm::mat4& matrix) = 0;

		static Ref<Shader> Create(const std::string& filepath);
		static Ref<Shader> Create(const std::string& name, const std::string& filepath);
		static Ref<Shader> Create(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath);
	};

}