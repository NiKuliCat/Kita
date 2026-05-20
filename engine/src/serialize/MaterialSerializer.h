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
			root["version"] = 2;
			root["shader"] = JsonUtils::SerializeAssetHandle(materialAsset.ShaderHandle);

			json textures;
			textures["albedo"] = JsonUtils::SerializeAssetHandle(materialAsset.m_Textures.Albedo);
			textures["normal"] = JsonUtils::SerializeAssetHandle(materialAsset.m_Textures.Normal);
			textures["metallicRoughness"] = JsonUtils::SerializeAssetHandle(materialAsset.m_Textures.MetallicRoughness);
			textures["ambientOcclusion"] = JsonUtils::SerializeAssetHandle(materialAsset.m_Textures.AmbientOcclusion);
			textures["emissive"] = JsonUtils::SerializeAssetHandle(materialAsset.m_Textures.Emissive);
			textures["opacity"] = JsonUtils::SerializeAssetHandle(materialAsset.m_Textures.Opacity);
			root["textures"] = std::move(textures);

			json params;
			params["baseColor"] = JsonUtils::SerializeVec4(materialAsset.m_SurfaceParams.BaseColor);
			params["emissive"] = JsonUtils::SerializeVec3(materialAsset.m_SurfaceParams.Emissive);
			params["metallic"] = materialAsset.m_SurfaceParams.Metallic;
			params["roughness"] = materialAsset.m_SurfaceParams.Roughness;
			params["ambientOcclusion"] = materialAsset.m_SurfaceParams.AmbientOcclusion;
			params["opacity"] = materialAsset.m_SurfaceParams.Opacity;
			params["normalScale"] = materialAsset.m_SurfaceParams.NormalScale;
			params["alphaCutoff"] = materialAsset.m_SurfaceParams.AlphaCutoff;
			root["params"] = std::move(params);

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

			materialAsset.m_Textures = {};
			materialAsset.m_SurfaceParams = {};

			if (root.contains("textures") && root.at("textures").is_object())
			{
				const json& textures = root.at("textures");
				if (textures.contains("albedo"))
					materialAsset.m_Textures.Albedo = JsonUtils::DeserializeAssetHandle(textures.at("albedo"));
				if (textures.contains("normal"))
					materialAsset.m_Textures.Normal = JsonUtils::DeserializeAssetHandle(textures.at("normal"));
				if (textures.contains("metallicRoughness"))
					materialAsset.m_Textures.MetallicRoughness = JsonUtils::DeserializeAssetHandle(textures.at("metallicRoughness"));
				if (textures.contains("ambientOcclusion"))
					materialAsset.m_Textures.AmbientOcclusion = JsonUtils::DeserializeAssetHandle(textures.at("ambientOcclusion"));
				if (textures.contains("emissive"))
					materialAsset.m_Textures.Emissive = JsonUtils::DeserializeAssetHandle(textures.at("emissive"));
				if (textures.contains("opacity"))
					materialAsset.m_Textures.Opacity = JsonUtils::DeserializeAssetHandle(textures.at("opacity"));
			}
			else if (root.contains("albedoTexture"))
			{
				// Backward compatibility for legacy material files.
				materialAsset.m_Textures.Albedo = JsonUtils::DeserializeAssetHandle(root.at("albedoTexture"));
			}

			if (root.contains("params") && root.at("params").is_object())
			{
				const json& params = root.at("params");
				if (params.contains("baseColor"))
					materialAsset.m_SurfaceParams.BaseColor = JsonUtils::DeserializeVec4(params.at("baseColor"));
				if (params.contains("emissive"))
					materialAsset.m_SurfaceParams.Emissive = JsonUtils::DeserializeVec3(params.at("emissive"));
				if (params.contains("metallic"))
					materialAsset.m_SurfaceParams.Metallic = params.at("metallic").get<float>();
				if (params.contains("roughness"))
					materialAsset.m_SurfaceParams.Roughness = params.at("roughness").get<float>();
				if (params.contains("ambientOcclusion"))
					materialAsset.m_SurfaceParams.AmbientOcclusion = params.at("ambientOcclusion").get<float>();
				if (params.contains("opacity"))
					materialAsset.m_SurfaceParams.Opacity = params.at("opacity").get<float>();
				if (params.contains("normalScale"))
					materialAsset.m_SurfaceParams.NormalScale = params.at("normalScale").get<float>();
				if (params.contains("alphaCutoff"))
					materialAsset.m_SurfaceParams.AlphaCutoff = params.at("alphaCutoff").get<float>();
			}
			else if (root.contains("baseColor"))
			{
				// Backward compatibility for legacy material files.
				materialAsset.m_SurfaceParams.BaseColor = JsonUtils::DeserializeVec4(root.at("baseColor"));
			}

			return true;
		}
	};
}
