#include "kita_pch.h"
#include "Asset.h"
#include "AssetManager.h"
#include "render/Material.h"
#include "render/ShaderLibrary.h"
#include "render/TextureLibrary.h"
namespace Kita{


	
	Ref<Material> MaterialAsset::CreateRuntimeMaterial() const
	{
		Ref<Material> material = CreateRef<Material>();
		ApplyToRuntimeMaterial(*material);
		return material;
	}

	void MaterialAsset::ApplyToRuntimeMaterial(Material& material) const
	{
		material.SetBaseColor(BaseColor);

		if (Asset::IsValidHandle(ShaderHandle))
		{
			auto shaderAsset = AssetManager::GetInstance().GetShaderAsset(ShaderHandle);
			if (shaderAsset)
			{
				material.SetShader(shaderAsset->GetFilePath().string());
			}
			else
			{
				material.ClearShader();
			}
		}
		else
		{
			material.ClearShader();
		}

		if (Asset::IsValidHandle(AlbedoTextureHandle))
		{
			auto textureAsset = AssetManager::GetInstance().GetTextureAsset(AlbedoTextureHandle);
			if (textureAsset)
			{
				material.SetAlbedoTexture(textureAsset->GetFilePath().string());
			}
			else
			{
				material.ClearAlbedoTexture();
			}
		}
		else
		{
			material.ClearAlbedoTexture();
		}
	}


	void ShaderAsset::SetShaderPath(const std::filesystem::path& path)
	{
		m_ShaderPath = path;
		m_RuntimeShader = ShaderLibrary::GetInstance().Load(m_ShaderPath.string());
	}

	void TextureAsset::SetTexturePath(const std::filesystem::path& path)
	{
		m_TexturePath = path;
		m_RuntimeTexture = TextureLibrary::GetInstance().Load(m_TexturePath.string());
	}

}
