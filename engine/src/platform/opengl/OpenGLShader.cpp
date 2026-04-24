#include "kita_pch.h"
#include "OpenGLShader.h"
#include "OpenGLUtil.h"
#include "core/Log.h"
#include <glm/gtc/type_ptr.hpp>

namespace Kita {

	OpenGLShader::OpenGLShader(const std::string& filepath)
	{
		m_Name = OpenGLUtil::GetFileNameWithoutExtension(filepath);
		KITA_CORE_INFO("Reader Shader File:{0}", m_Name);

		std::unordered_map<GLenum, std::string> src = OpenGLUtil::GLSLReader(filepath);
		uint32_t program = glCreateProgram();

		uint32_t vs = CompileShader(GL_VERTEX_SHADER, src[GL_VERTEX_SHADER]);
		uint32_t fs = CompileShader(GL_FRAGMENT_SHADER, src[GL_FRAGMENT_SHADER]);
		if (vs == 0 || fs == 0)
		{
			if (vs != 0) glDeleteShader(vs);
			if (fs != 0) glDeleteShader(fs);
			glDeleteProgram(program);
			KITA_CORE_ASSERT(false, "OpenGL shader compile failed: {0}", m_Name);
			return;
		}

		glAttachShader(program, vs);
		glAttachShader(program, fs);
		glLinkProgram(program);
		GLint linked = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &linked);
		if (linked == GL_FALSE)
		{
			GLint len = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
			std::string log;
			log.resize((size_t)std::max(len, 1));
			glGetProgramInfoLog(program, len, &len, log.data());
			KITA_CORE_ERROR("Shader link failed: {0}", m_Name);
			KITA_CORE_ERROR("{0}", log);
			KITA_CORE_ASSERT(false, "OpenGL program link failed.");
		}
		glValidateProgram(program);
		glDeleteShader(vs);
		glDeleteShader(fs);

		m_ShaderID = program;
		
	}


	OpenGLShader::OpenGLShader(const std::string& name, const std::string& filepath)
		:m_Name(name)
	{
		KITA_CORE_INFO("Reader Shader File:{0}", m_Name);

		std::unordered_map<GLenum, std::string> src = OpenGLUtil::GLSLReader(filepath);
		uint32_t program = glCreateProgram();

		uint32_t vs = CompileShader(GL_VERTEX_SHADER, src[GL_VERTEX_SHADER]);
		uint32_t fs = CompileShader(GL_FRAGMENT_SHADER, src[GL_FRAGMENT_SHADER]);
		if (vs == 0 || fs == 0)
		{
			if (vs != 0) glDeleteShader(vs);
			if (fs != 0) glDeleteShader(fs);
			glDeleteProgram(program);
			KITA_CORE_ASSERT(false, "OpenGL shader compile failed: {0}", m_Name);
			return;
		}

		glAttachShader(program, vs);
		glAttachShader(program, fs);
		glLinkProgram(program);
		GLint linked = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &linked);
		if (linked == GL_FALSE)
		{
			GLint len = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
			std::string log;
			log.resize((size_t)std::max(len, 1));
			glGetProgramInfoLog(program, len, &len, log.data());
			KITA_CORE_ERROR("Shader link failed: {0}", m_Name);
			KITA_CORE_ERROR("{0}", log);
			KITA_CORE_ASSERT(false, "OpenGL program link failed.");
		}
		glValidateProgram(program);
		glDeleteShader(vs);
		glDeleteShader(fs);

		m_ShaderID = program;
	}
	OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath)
		:m_Name(name)
	{
		std::string	 vertexSource = OpenGLUtil::GLSLReader(ShaderType::Vertex, vertexPath);
		std::string	 fragmentSource = OpenGLUtil::GLSLReader(ShaderType::Fragment, fragmentPath);
		uint32_t program = glCreateProgram();
		uint32_t vs = CompileShader(GL_VERTEX_SHADER, vertexSource);
		uint32_t fs = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);
		if (vs == 0 || fs == 0)
		{
			if (vs != 0) glDeleteShader(vs);
			if (fs != 0) glDeleteShader(fs);
			glDeleteProgram(program);
			KITA_CORE_ASSERT(false, "OpenGL shader compile failed: {0}", m_Name);
			return;
		}
		glAttachShader(program, vs);
		glAttachShader(program, fs);
		glLinkProgram(program);
		GLint linked = GL_FALSE;
		glGetProgramiv(program, GL_LINK_STATUS, &linked);
		if (linked == GL_FALSE)
		{
			GLint len = 0;
			glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
			std::string log;
			log.resize((size_t)std::max(len, 1));
			glGetProgramInfoLog(program, len, &len, log.data());
			KITA_CORE_ERROR("Shader link failed: {0}", m_Name);
			KITA_CORE_ERROR("{0}", log);
			KITA_CORE_ASSERT(false, "OpenGL program link failed.");
		}
		glValidateProgram(program);
		glDeleteShader(vs);
		glDeleteShader(fs);
		m_ShaderID = program;
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
		Bind();
		GLint location = glGetUniformLocation(m_ShaderID, name.c_str());
		glUniform1i(location, value);
	}

	void OpenGLShader::SetVector(const std::string& name, const glm::vec4& value)
	{
		Bind();
		GLint location = glGetUniformLocation(m_ShaderID, name.c_str());
		glUniform4f(location, value.x, value.y, value.z, value.w);
	}

	void OpenGLShader::SetMat4(const std::string& name, const glm::mat4& matrix)
	{
		Bind();
		GLint location = glGetUniformLocation(m_ShaderID, name.c_str());
		glUniformMatrix4fv(location, 1, GL_FALSE, &matrix[0][0]);
	}

	void OpenGLShader::SetColor(const std::string& name, const glm::vec4& color)
	{
		Bind();
		GLint location = glGetUniformLocation(m_ShaderID, name.c_str());
		glUniform4fv(location, 1, glm::value_ptr(color));
	}

	uint32_t OpenGLShader::CompileShader(uint32_t type, const std::string& source)
	{
		if (source.empty())
		{
			KITA_CORE_ERROR("{0} shader source is empty", type == GL_VERTEX_SHADER ? "vertex" : "fragment");
			return 0;
		}

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
			std::string log;
			log.resize((size_t)std::max(len, 1));
			glGetShaderInfoLog(id, len, &len, log.data());

			KITA_CORE_ERROR("{0} shader is failed to compile", type == GL_VERTEX_SHADER ? "vertex" : "fragment");
			KITA_CORE_ERROR("{0}", log);

			glDeleteShader(id);
			return 0;
		}

		return id;
	}

}
