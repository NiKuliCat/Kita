#pragma once

#include "asset/AssetManager.h"

namespace Kita::RenderAssetUtils
{
	inline Ref<Shader> GetRuntimeShader(const std::filesystem::path& assetPath)
	{
		auto& assetManager = AssetManager::GetInstance();
		const AssetHandle handle = assetManager.GetHandleByPath(assetPath);
		if (!Asset::IsValidHandle(handle))
		{
			return nullptr;
		}

		Ref<ShaderAsset> shaderAsset = assetManager.GetShaderAsset(handle);
		if (!shaderAsset)
		{
			return nullptr;
		}

		return shaderAsset->GetRuntimeShader();
	}
}
