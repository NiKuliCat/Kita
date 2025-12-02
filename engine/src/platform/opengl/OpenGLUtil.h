#pragma once
#include <glad/glad.h>
#include "render/Shader.h"
namespace Kita {

	class OpenGLUtil
	{
	public:

		static std::unordered_map<GLenum, std::string> GLSLReader(const std::string& filepath);
		static std::string GLSLReader(const ShaderType type, const std::string& filepath);
		static std::string GetFileNameWithoutExtension(const std::string& filepath);

	private :
		inline static std::string Trim(const std::string& str);
	};
}