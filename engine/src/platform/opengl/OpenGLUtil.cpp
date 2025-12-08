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
	/*bool OpenGLUtil::IsDepthFormat(const FrameBufferTexFormat format)
	{
		if (format == FrameBufferTexFormat::DEPTH24STENCIL8)   return true;
		return false;
	}*/
	inline std::string OpenGLUtil::Trim(const std::string& str)
	{
		size_t start = str.find_first_not_of(" \t");
		if (start == std::string::npos) return "";
		size_t end = str.find_last_not_of(" \t");
		return str.substr(start, end - start + 1);
	}
}