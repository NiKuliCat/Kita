#include "kita_pch.h"
#include "Shader.h"
#include "RendererAPI.h"
#include "platform/opengl/OpenGLShader.h"
namespace Kita {

	 Ref<Shader> Shader::Create(const std::string& filepath)
	{
		switch (RendererAPI::GetAPI())
		{
		case  RendererAPI::API::None:
		{
			return nullptr;
			break;
		}
		case  RendererAPI::API::OpenGL:
		{
			return CreateRef<OpenGLShader>(filepath);
			break;
		}
		}
	}


	Ref<Shader> Shader::Create(const std::string& vertexPath, const std::string& fragmentPath)
	{
		switch (RendererAPI::GetAPI())
		{
		case  RendererAPI::API::None:
		{
			return nullptr;
			break;
		}
		case  RendererAPI::API::OpenGL:
		{
			return CreateRef<OpenGLShader>(vertexPath,fragmentPath);
			break;
		}
		}
	}

}