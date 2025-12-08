#pragma once
#include <glad/glad.h>
#include "render/Shader.h"
#include <render/FrameBuffer.h>
namespace Kita {

	class OpenGLUtil
	{
	public:


		//shader
		static std::unordered_map<GLenum, std::string> GLSLReader(const std::string& filepath);
		static std::string GLSLReader(const ShaderType type, const std::string& filepath);
		static std::string GetFileNameWithoutExtension(const std::string& filepath);


		//framebuffer
		//static bool IsDepthFormat(const FrameBufferTexFormat format);

		

	private :
		inline static std::string Trim(const std::string& str);
	};
}