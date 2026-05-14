#pragma once

#include <EngineCore.h>

namespace Kita {

	inline std::string GetAssetEditorDisplayName(AssetHandle handle)
	{
		if (!Asset::IsValidHandle(handle))
		{
			return "None";
		}

		if (const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(handle))
		{
			const std::string filename = metadata->relativePath.filename().string();
			if (!filename.empty())
			{
				return filename;
			}

			return metadata->relativePath.generic_string();
		}

		return "None";
	}

}
