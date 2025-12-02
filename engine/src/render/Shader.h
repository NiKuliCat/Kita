#pragma once
#include "core/Core.h"

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

		static Ref<Shader> Create(const std::string& filepath);
		static Ref<Shader> Create(const std::string& vertexPath, const std::string& fragmentPath);
	};

}