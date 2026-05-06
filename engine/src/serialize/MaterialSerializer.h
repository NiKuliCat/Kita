#pragma once
#include "asset/Asset.h"
#include "core/Log.h"
#include "serialize/JsonUtils.h"

namespace Kita {

	class MaterialSerializer
	{
	public:
		static bool Serialize(const std::filesystem::path& path, const MaterialAsset& materialAsset)
		{
			json root;
			root["version"] = 1;
			root["shader"] = JsonUtils::SerializeAssetHandle(materialAsset.ShaderHandle);
			root["albedoTexture"] = JsonUtils::SerializeAssetHandle(materialAsset.AlbedoTextureHandle);
			root["baseColor"] = JsonUtils::SerializeVec4(materialAsset.BaseColor);

			std::ofstream out(path);
			if (!out.is_open())
			{
				KITA_CORE_ERROR("Failed to open material file for write: {0}", path.string());
				return false;
			}

			out << root.dump(4);
			return true;
		}

		static bool Deserialize(const std::filesystem::path& path, MaterialAsset& materialAsset)
		{
			std::ifstream in(path);
			if (!in.is_open())
			{
				KITA_CORE_ERROR("Failed to open material file for read: {0}", path.string());
				return false;
			}

			json root;
			in >> root;

			if (!root.is_object())
			{
				KITA_CORE_ERROR("Material file root must be a json object: {0}", path.string());
				return false;
			}

			if (!root.contains("shader"))
			{
				KITA_CORE_WARN("Material file missing 'shader': {0}", path.string());
				materialAsset.ShaderHandle = InvalidAssetHandle;
			}
			else
			{
				materialAsset.ShaderHandle = JsonUtils::DeserializeAssetHandle(root.at("shader"));
			}

			if (!root.contains("albedoTexture"))
			{
				KITA_CORE_WARN("Material file missing 'albedoTexture': {0}", path.string());
				materialAsset.AlbedoTextureHandle = InvalidAssetHandle;
			}
			else
			{
				materialAsset.AlbedoTextureHandle = JsonUtils::DeserializeAssetHandle(root.at("albedoTexture"));
			}

			if (root.contains("baseColor"))
			{
				materialAsset.BaseColor = JsonUtils::DeserializeVec4(root["baseColor"]);
			}
			else
			{
				materialAsset.BaseColor = glm::vec4(1.0f);
			}

			return true;
		}
	};
}
