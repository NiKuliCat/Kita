#pragma once

#include "Shader.h"
#include "Texture.h"
namespace Kita {


	class Material
	{
	public:
		Material() = default;
		Material(const std::string& shader_path, const std::string& tex_path);

		Ref<Shader>& GetShader() { return m_Shader; }
	private:
		Ref<Shader> m_Shader = nullptr;
		Ref<Texture> m_Texture = nullptr;
	};
}