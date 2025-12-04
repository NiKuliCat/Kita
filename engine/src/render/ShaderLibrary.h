#pragma once
#include "core/Core.h"
#include "Shader.h"

namespace Kita {

	class ShaderLibrary
	{
	public:

		static  ShaderLibrary& GetInstance();
		void Add(const Ref<Shader>& shader);
		Ref<Shader> Load(const std::string& filepath);
		Ref<Shader> Load(const std::string& name, const std::string& filepath);
		Ref<Shader> Get(const std::string& name);
		bool Exists(const std::string& name) const;

		
		const std::vector<std::string> GetShaderNames() const;


	private:
		ShaderLibrary() = default;


	private:
		std::unordered_map<std::string, Ref<Shader>> m_Shaders;
	};
}
