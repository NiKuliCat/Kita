#include "kita_pch.h"

#include "OpenGLUtil.h"

namespace Kita {

	std::unordered_map<GLenum, std::string> OpenGLUtil::GLSLReader(const std::string& filepath)
	{
		std::unordered_map<GLenum, std::string> shaderSources;
		std::ifstream file(filepath);
		std::string line;
		
		std::stringstream vertex;
		std::stringstream fragment;
		std::string target = "#program";

		bool reading = false;
		ShaderType curProcessType = ShaderType::Vertex;
		while (std::getline(file,line))
		{
			//消除注释
			size_t commentPos = line.find("//");
			if (commentPos != std::string::npos) {
				line = line.substr(0, commentPos);
			}
			//先判断该行是否为标签行
				//消除首尾空位
			std::string trimLine = Trim(line);
			if (trimLine.empty()) continue;

			//寻找标签值
			if (trimLine.find(target) == 0)
			{
				std::string subLine = trimLine.substr(target.length());
				subLine = Trim(subLine);
				if (subLine == "vertex")
				{
					curProcessType = ShaderType::Vertex;
					reading = true;
					continue;
				}
				else if (subLine == "fragment")
				{
					curProcessType = ShaderType::Fragment;
					reading = true;
					continue;
				}
			}
			//如果成功进入了读取阶段
			if (reading)
			{
				switch (curProcessType)
				{
					case ShaderType::Vertex:
					{
						vertex << line << '\n';
						break;
					}
					case ShaderType::Fragment:
					{
						fragment << line << '\n';
						break;
					}
				}
				continue;
			}
		}

		shaderSources[GL_VERTEX_SHADER] = vertex.str();
		shaderSources[GL_FRAGMENT_SHADER] = fragment.str();

		return shaderSources;
	}
	std::string OpenGLUtil::GLSLReader(const ShaderType type, const std::string& filepath)
	{
		std::ifstream file(filepath);
		std::string line;
		std::stringstream shaderSources;
		bool enableRead = false;
		while (std::getline(file, line))
		{
			if ((line.find("#vertex") != std::string::npos && type == ShaderType::Vertex)
				|| (line.find("#fragment") != std::string::npos && type == ShaderType::Fragment))
			{
				enableRead = true;
				continue;
			}
			else if ((line.find("#vertex") != std::string::npos && type != ShaderType::Vertex)
				|| (line.find("#fragment") != std::string::npos && type != ShaderType::Fragment))
			{
				enableRead = false;
			}

			if (enableRead)
			{
				shaderSources << line << '\n';
			}

		}
		return shaderSources.str();
	}
	std::string OpenGLUtil::GetFileNameWithoutExtension(const std::string& filepath)
	{
		std::filesystem::path path(filepath);
		return path.stem().string();
	}
	bool OpenGLUtil::IsDepthFormat(const FrameBufferTexFormat format)
	{
		if (format == FrameBufferTexFormat::DEPTH24STENCIL8)   return true;
		return false;
	}
	void OpenGLUtil::CreateAttachment(bool multisample, uint32_t count, uint32_t* out_id)
	{
		glCreateTextures(TextureTarget(multisample), count, out_id);
	}
	void OpenGLUtil::BindAttachment(bool multisample, uint32_t id)
	{
		glBindTexture(TextureTarget(multisample), id);
	}
	void OpenGLUtil::AttachColorTexture(uint32_t id, int sample, GLenum internalFormat, GLenum format, uint32_t width, uint32_t height, GLenum type, int index)
	{
		bool multisampled = sample > 1;
		if (multisampled)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sample, format, width, height, GL_FALSE);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, type, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			// 可选：设置 Wrap 方式
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + index, TextureTarget(multisampled), id, 0);
	}
	void OpenGLUtil::AttachColorTexture(FrameBufferTexFormat& format, uint32_t id, int sample, uint32_t width, uint32_t height, int index)
	{
		switch (format)
		{
			case FrameBufferTexFormat::RGBA8:
			{
				AttachColorTexture(id, sample, GL_RGBA8, GL_RGBA, width, height, GL_UNSIGNED_BYTE, index);
				break;
			}
			case FrameBufferTexFormat::RED_INTEGER:
			{
				AttachColorTexture(id, sample, GL_R32I, GL_RED_INTEGER, width, height, GL_INT, index);
				break;
			}
			case FrameBufferTexFormat::RGBA16F:
			{
				AttachColorTexture(id, sample, GL_RGBA16F, GL_RGBA, width, height, GL_FLOAT, index);
				break;
			}
		}
	}
	void OpenGLUtil::AttachDepthTexture(uint32_t id, int samples, GLenum format, GLenum depthAttachType, uint32_t width, uint32_t height)
	{
		bool multisampled = samples > 1;
		if (multisampled)
		{
			glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, samples, format, width, height, GL_FALSE);
		}
		else
		{
			glTexStorage2D(GL_TEXTURE_2D, 1, format, width, height);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		glFramebufferTexture2D(GL_FRAMEBUFFER, depthAttachType, TextureTarget(multisampled), id, 0);
	}
	void OpenGLUtil::AttachDepthTexture(FrameBufferTexFormat& format, uint32_t id, int samples, uint32_t width, uint32_t height)
	{
		switch (format)
		{
			case FrameBufferTexFormat::DEPTH24STENCIL8:
			{
				AttachDepthTexture(id, samples, GL_DEPTH24_STENCIL8, GL_DEPTH_STENCIL_ATTACHMENT, width, height);
				break;
			}
		}
	}
	GLenum OpenGLUtil::TextureTarget(bool multisampled)
	{
		return multisampled ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
	}
	inline std::string OpenGLUtil::Trim(const std::string& str)
	{
		size_t start = str.find_first_not_of(" \t");
		if (start == std::string::npos) return "";
		size_t end = str.find_last_not_of(" \t");
		return str.substr(start, end - start + 1);
	}
}