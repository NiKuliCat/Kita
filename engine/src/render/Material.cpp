#include "kita_pch.h"
#include "Material.h"
#include "asset/AssetManager.h"

namespace Kita {


	Material::Material(const std::string& shader_path, const std::string& tex_path)
	{
		SetShader(shader_path);
		SetAlbedoTexture(tex_path);
	}

	void Material::SetShader(const Ref<Shader>& shader)
	{
		m_Shader = shader;
	}

	void Material::SetShader(const std::string& path)
	{
		if (path.empty())
		{
			ClearShader();
			return;
		}

		auto& assetManager = AssetManager::GetInstance();
		const AssetHandle shaderHandle = assetManager.GetHandleByPath(path);
		if (!Asset::IsValidHandle(shaderHandle))
		{
			m_Shader = nullptr;
			return;
		}

		Ref<ShaderAsset> shaderAsset = assetManager.GetShaderAsset(shaderHandle);
		m_Shader = shaderAsset ? shaderAsset->GetRuntimeShader() : nullptr;
	}

	void Material::ClearShader()
	{
		m_Shader = nullptr;
	}

	void Material::SetAlbedoTexture(const Ref<Texture>& texture)
	{
		m_AlbedoTex = texture;
	}

	void Material::SetAlbedoTexture(const std::string& path)
	{
		if (path.empty())
		{
			ClearAlbedoTexture();
			return;
		}

		auto& assetManager = AssetManager::GetInstance();
		const AssetHandle textureHandle = assetManager.GetHandleByPath(path);
		if (!Asset::IsValidHandle(textureHandle))
		{
			m_AlbedoTex = nullptr;
			return;
		}

		Ref<TextureAsset> textureAsset = assetManager.GetTextureAsset(textureHandle);
		m_AlbedoTex = textureAsset ? textureAsset->GetRuntimeTexture() : nullptr;
	}

	void Material::ClearAlbedoTexture()
	{
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
