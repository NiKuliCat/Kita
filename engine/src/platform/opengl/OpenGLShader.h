#pragma once

#include "render/Shader.h"

#include <glad/glad.h>


namespace Kita {

	class OpenGLShader : public Shader
	{
	public:
		OpenGLShader(const std::string& filepath);
		OpenGLShader(const std::string& vertexPath,const std::string& fragmentPath);

		virtual ~OpenGLShader();


		virtual void Bind() override;
		virtual void UnBind() override;
		
		virtual const uint32_t GetID() const override { return m_ShaderID; }
		virtual const std::string& GetName() const override { return m_Name; }


	private:
		uint32_t CompileShader(uint32_t type, const std::string& source);


	private:
		uint32_t m_ShaderID;
		std::string m_Name;
		
	};
}