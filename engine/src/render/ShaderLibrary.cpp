#include "kita_pch.h"

#include "ShaderLibrary.h"
#include "core/Log.h"
namespace Kita {
	void ShaderLibrary::Add(const Ref<Shader>& shader)
	{
		if (shader)
		{
			auto name = shader->GetName();
			if (Exists(name))
			{
				KITA_CORE_WARN("Shader already exists in ShaderLibrary : {0}", name);
			}
			else
			{
				m_Shaders[name] = shader;
			}
		}
	}
	Ref<Shader> ShaderLibrary::Load(const std::string& filepath)
	{
		Ref<Shader> shader = Shader::Create(filepath);
		Add(shader);

		return shader;

	}
	Ref<Shader> ShaderLibrary::Load(const std::string& name, const std::string& filepath)
	{
		Ref<Shader> shader = Shader::Create(name,filepath);
		Add(shader);

		return shader;
	}
	Ref<Shader> ShaderLibrary::Get(const std::string& name)
	{
		if(Exists(name))
		{
			return m_Shaders[name];
		}
		else
		{
			KITA_CORE_ERROR(" the shader: {0} don`t exists in ShaderLibrary : {0}", name);
			return nullptr;
		}
	}
	bool ShaderLibrary::Exists(const std::string& name) const
	{
		return m_Shaders.find(name) != m_Shaders.end();
	}
	const std::vector<std::string> ShaderLibrary::GetShaderNames() const
	{
		std::vector<std::string> names;

		names.reserve(m_Shaders.size());

		for( const auto& shader : m_Shaders)
		{
			names.push_back(shader.first);
		}
		return names;
	}
	ShaderLibrary& ShaderLibrary::GetInstance()
	{
		static ShaderLibrary instance;
		return instance;
	}


}