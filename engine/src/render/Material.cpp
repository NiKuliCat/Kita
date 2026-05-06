#include "kita_pch.h"
#include "Material.h"
#include "ShaderLibrary.h"
#include "TextureLibrary.h"

namespace Kita {


	Material::Material(const std::string& shader_path, const std::string& tex_path)
	{
		auto& shaderLibrary = ShaderLibrary::GetInstance();
		m_Shader = shaderLibrary.Load(shader_path);
	}

	void Material::SetShader(const std::string& path)
	{
		if (path.empty())
		{
			ClearShader();
			return;
		}

		m_ShaderFilePath = path;
		m_Shader = ShaderLibrary::GetInstance().Load(m_ShaderFilePath);
	}

	void Material::ClearShader()
	{
		m_ShaderFilePath.clear();
		m_Shader = nullptr;
	}

	void Material::SetAlbedoTexture(const std::string& path)
	{
		if (path.empty())
		{
			ClearAlbedoTexture();
			return;
		}

		m_AlbedoTexPath = path;
		m_AlbedoTex = TextureLibrary::GetInstance().Load(m_AlbedoTexPath);
	}

	void Material::ClearAlbedoTexture()
	{
		m_AlbedoTexPath.clear();
		m_AlbedoTex = nullptr;
	}

	void Material::Bind()
	{
		if (!m_Shader)
			return;

		if (m_AlbedoTex)
		{
			m_AlbedoTex->Bind(3);
			m_Shader->SetInt("MainTex", 3);
		}

		m_Shader->SetColor("BaseColor", m_BaseColor);
	}

}
