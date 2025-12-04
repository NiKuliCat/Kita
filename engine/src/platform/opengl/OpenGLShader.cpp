#include "kita_pch.h"
#include "OpenGLShader.h"
#include "OpenGLUtil.h"
#include "core/Log.h"
namespace Kita {

	OpenGLShader::OpenGLShader(const std::string& filepath)
	{
		m_Name = OpenGLUtil::GetFileNameWithoutExtension(filepath);
		KITA_CORE_INFO("Reader Shader File:{0}", m_Name);

		std::unordered_map<GLenum, std::string> src = OpenGLUtil::GLSLReader(filepath);

		KITA_CORE_INFO("Vertex Shader Source:\n{0}", src[GL_VERTEX_SHADER]);
		KITA_CORE_INFO("Fragment Shader Source:\n{0}", src[GL_FRAGMENT_SHADER]);
		uint32_t program = glCreateProgram();

		uint32_t vs = CompileShader(GL_VERTEX_SHADER, src[GL_VERTEX_SHADER]);
		uint32_t fs = CompileShader(GL_FRAGMENT_SHADER, src[GL_FRAGMENT_SHADER]);

		glAttachShader(program, vs);
		glAttachShader(program, fs);
		glLinkProgram(program);
		glValidateProgram(program);
		glDeleteShader(vs);
		glDeleteShader(fs);

		m_ShaderID = program;
		
	}

	OpenGLShader::OpenGLShader(const std::string& vertexPath, const std::string& fragmentPath)
	{
		std::string	 vertexSource = OpenGLUtil::GLSLReader(ShaderType::Vertex, vertexPath);
		std::string	 fragmentSource = OpenGLUtil::GLSLReader(ShaderType::Fragment, fragmentPath);
		uint32_t program = glCreateProgram();
		uint32_t vs = CompileShader(GL_VERTEX_SHADER, vertexSource);
		uint32_t fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
		glAttachShader(program, vs);
		glAttachShader(program, fs);
		glLinkProgram(program);
		glValidateProgram(program);
		glDeleteShader(vs);
		glDeleteShader(fs);
		m_ShaderID = program;
		m_Name = OpenGLUtil::GetFileNameWithoutExtension(vertexPath);
	}

	OpenGLShader::~OpenGLShader()
	{
	}

	void OpenGLShader::Bind()
	{
		glUseProgram(m_ShaderID);
	}

	void OpenGLShader::UnBind()
	{
		glUseProgram(0);
	}

	void OpenGLShader::SetInt(const std::string& name, const uint32_t value)
	{
		GLint location = glGetUniformLocation(m_ShaderID, name.c_str());
		glUniform1i(location, value);
	}

	uint32_t OpenGLShader::CompileShader(uint32_t type, const std::string& source)
	{
		uint32_t id = glCreateShader(type);
		const char* shaderSrc = source.c_str();
		glShaderSource(id, 1, &shaderSrc, nullptr);
		glCompileShader(id);
		int result;
		glGetShaderiv(id, GL_COMPILE_STATUS, &result);


		if (result == GL_FALSE)
		{
			int len;
			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &len);
			char* log = (char*)malloc(len * sizeof(char));
			glGetShaderInfoLog(id, len, &len, log);

			KITA_CORE_ERROR("{0} shader is failed to compile", type == GL_VERTEX_SHADER ? "vertex" : "fragment");
			KITA_CORE_ERROR("{0}", log);

			glDeleteShader(id);
			return 0;
		}

		return id;
	}

}