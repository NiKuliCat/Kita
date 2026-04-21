#include "kita_pch.h"
#include "Material.h"
#include "ShaderLibrary.h"

namespace Kita {


	Material::Material(const std::string& shader_path, const std::string& tex_path)
	{
		auto& shaderLibrary = ShaderLibrary::GetInstance();

		m_Shader = shaderLibrary.Load(shader_path);
		TextureDescriptor texDesc{};
		m_Texture = Texture::Create(texDesc, tex_path);

		m_Texture->Bind(3);
		m_Shader->SetInt("MainTex", 3);
	}

}