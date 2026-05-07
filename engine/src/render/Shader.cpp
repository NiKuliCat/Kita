#include "kita_pch.h"
#include "Shader.h"
namespace Kita {

	 Ref<Shader> Shader::Create(const std::string& filepath)
	{
		(void)filepath;
		throw std::runtime_error("Legacy Shader::Create(path) path is disabled during Vulkan-only migration.");
		return nullptr;
	}

	Ref<Shader>  Shader::Create(const std::string& name, const std::string& filepath)
	{
		(void)name;
		(void)filepath;
		throw std::runtime_error("Legacy Shader::Create(name, path) path is disabled during Vulkan-only migration.");
		return nullptr;
	}
	Ref<Shader>  Shader::Create(const std::string& name, const std::string& vertexPath, const std::string& fragmentPath)
	{
		(void)name;
		(void)vertexPath;
		(void)fragmentPath;
		throw std::runtime_error("Legacy Shader::Create(vs, fs) path is disabled during Vulkan-only migration.");
		return nullptr;
	}

}
