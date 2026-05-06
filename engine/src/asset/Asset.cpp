#include "kita_pch.h"
#include "Asset.h"
#include  "render/ShaderLibrary.h"
#include "render/TextureLibrary.h"
namespace Kita{


	void ShaderAsset::SetShaderPath(const std::filesystem::path& path)
	{
		m_ShaderPath = path;
		m_RuntimeShader = ShaderLibrary::GetInstance().Get(path.string());
	}

	void TextureAsset::SetTexturePath(const std::filesystem::path& path)
	{
		m_TexturePath = path;
		m_RuntimeTexture = TextureLibrary::GetInstance().Load(m_TexturePath.string());
	}
}
