#include "kita_pch.h"
#include "Material.h"
#include "ShaderLibrary.h"

namespace Kita {


	Material::Material(const std::string& shader_path, const std::string& tex_path)
	{
		auto& shaderLibrary = ShaderLibrary::GetInstance();
		m_Shader = shaderLibrary.Load(shader_path);
	}

	void Material::SetShader(const std::string& path)
	{
		m_ShaderFilePath = path;
		m_Shader = ShaderLibrary::GetInstance().Load(m_ShaderFilePath);
	}

	void Material::SetAlbedoTexture(const std::string& path)
	{
		TextureDescriptor texDesc{};
		m_AlbedoTexPath = path;
		m_AlbedoTex = Texture::Create(texDesc, m_AlbedoTexPath);
	}

	void Material::Bind()
	{
		m_AlbedoTex->Bind(3);
		m_Shader->SetInt("MainTex", 3);
		m_Shader->SetColor("BaseColor", m_BaseColor);
	}

}