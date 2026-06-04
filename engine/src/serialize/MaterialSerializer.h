#pragma once
#include "asset/Asset.h"
#include "core/Log.h"
#include "serialize/JsonUtils.h"

#include <fstream>
#include <variant>

namespace Kita {

	class MaterialSerializer
	{
	public:
		static bool Serialize(const std::filesystem::path& path, const MaterialAsset& materialAsset)
		{
			json root = json::object();
			root["version"] = 3;

			// 新版材质优先绑定 ShaderLab 资产。
			root["shaderLab"] = JsonUtils::SerializeAssetHandle(materialAsset.ShaderLabHandle);

			// 过渡阶段仍保留旧字段，便于排查和兼容旧工具链。
			// 后续整个运行时完全迁完后，可以再考虑移除。
			root["shader"] = JsonUtils::SerializeAssetHandle(materialAsset.ShaderHandle);

			root["properties"] = SerializePropertyMap(materialAsset.PropertyBlock.Values);
			root["orphans"] = SerializePropertyMap(materialAsset.PropertyBlock.OrphanValues);

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

			const uint32_t version = root.contains("version") && root.at("version").is_number_unsigned()
				? root.at("version").get<uint32_t>()
				: 1u;

			// 每次反序列化都先清空，避免旧数据残留。
			materialAsset.ShaderLabHandle = InvalidAssetHandle;
			materialAsset.ShaderHandle = InvalidAssetHandle;
			materialAsset.PropertyBlock = {};
			materialAsset.m_Textures = {};
			materialAsset.m_SurfaceParams = {};

			switch (version)
			{
			case 3:
				return DeserializeV3(root, path, materialAsset);

			case 2:
			case 1:
				return DeserializeLegacy(root, path, materialAsset);

			default:
				KITA_CORE_WARN(
					"Unknown material version {0}, fallback to legacy reader: {1}",
					version,
					path.string());
				return DeserializeLegacy(root, path, materialAsset);
			}
		}

	private:
		static const char* MaterialValueTypeToString(MaterialValueType type)
		{
			switch (type)
			{
			case MaterialValueType::Bool:        return "Bool";
			case MaterialValueType::Int:         return "Int";
			case MaterialValueType::Float:       return "Float";
			case MaterialValueType::Float2:      return "Float2";
			case MaterialValueType::Float3:      return "Float3";
			case MaterialValueType::Float4:      return "Float4";
			case MaterialValueType::Color:       return "Color";
			case MaterialValueType::Texture2D:   return "Texture2D";
			case MaterialValueType::TextureCube: return "TextureCube";
			default:                             return "Float";
			}
		}

		static bool TryParseMaterialValueType(const std::string& text, MaterialValueType& outType)
		{
			if (text == "Bool") { outType = MaterialValueType::Bool; return true; }
			if (text == "Int") { outType = MaterialValueType::Int; return true; }
			if (text == "Float") { outType = MaterialValueType::Float; return true; }
			if (text == "Float2") { outType = MaterialValueType::Float2; return true; }
			if (text == "Float3") { outType = MaterialValueType::Float3; return true; }
			if (text == "Float4") { outType = MaterialValueType::Float4; return true; }
			if (text == "Color") { outType = MaterialValueType::Color; return true; }
			if (text == "Texture2D") { outType = MaterialValueType::Texture2D; return true; }
			if (text == "TextureCube") { outType = MaterialValueType::TextureCube; return true; }
			return false;
		}

		static json SerializeVec2(const glm::vec2& value)
		{
			return json::array({ value.x, value.y });
		}

		static glm::vec2 DeserializeVec2(const json& value)
		{
			if (!value.is_array() || value.size() != 2)
			{
				return glm::vec2(0.0f);
			}

			return glm::vec2(
				value[0].get<float>(),
				value[1].get<float>());
		}

		static json SerializePropertyValue(const MaterialPropertyValue& value)
		{
			json propertyJson = json::object();
			propertyJson["kind"] = MaterialValueTypeToString(value.ValueType);

			switch (value.ValueType)
			{
			case MaterialValueType::Bool:
				propertyJson["value"] = std::get<bool>(value.Data);
				break;

			case MaterialValueType::Int:
				propertyJson["value"] = std::get<int32_t>(value.Data);
				break;

			case MaterialValueType::Float:
				propertyJson["value"] = std::get<float>(value.Data);
				break;

			case MaterialValueType::Float2:
				propertyJson["value"] = SerializeVec2(std::get<glm::vec2>(value.Data));
				break;

			case MaterialValueType::Float3:
				propertyJson["value"] = JsonUtils::SerializeVec3(std::get<glm::vec3>(value.Data));
				break;

			case MaterialValueType::Float4:
			case MaterialValueType::Color:
				propertyJson["value"] = JsonUtils::SerializeVec4(std::get<glm::vec4>(value.Data));
				break;

			case MaterialValueType::Texture2D:
			case MaterialValueType::TextureCube:
			{
				// 说明：
				// 1. 运行时正式材质实例建议使用 AssetHandle
				// 2. 若仍是默认纹理名字符串，也允许序列化为 name 字段
				if (std::holds_alternative<AssetHandle>(value.Data))
				{
					propertyJson["asset"] = JsonUtils::SerializeAssetHandle(std::get<AssetHandle>(value.Data));
				}
				else if (std::holds_alternative<std::string>(value.Data))
				{
					propertyJson["name"] = std::get<std::string>(value.Data);
				}
				else
				{
					propertyJson["asset"] = JsonUtils::SerializeAssetHandle(InvalidAssetHandle);
				}
				break;
			}

			default:
				propertyJson["value"] = 0.0f;
				break;
			}

			return propertyJson;
		}

		static bool DeserializePropertyValue(
			const json& propertyJson,
			MaterialPropertyValue& outValue,
			const std::filesystem::path& path,
			const std::string& propertyName)
		{
			if (!propertyJson.is_object())
			{
				KITA_CORE_WARN(
					"Material property '{0}' is not an object: {1}",
					propertyName,
					path.string());
				return false;
			}

			if (!propertyJson.contains("kind") || !propertyJson.at("kind").is_string())
			{
				KITA_CORE_WARN(
					"Material property '{0}' missing kind: {1}",
					propertyName,
					path.string());
				return false;
			}

			MaterialValueType type = MaterialValueType::Float;
			if (!TryParseMaterialValueType(propertyJson.at("kind").get<std::string>(), type))
			{
				KITA_CORE_WARN(
					"Material property '{0}' has unsupported kind: {1}",
					propertyName,
					path.string());
				return false;
			}

			outValue.ValueType = type;

			try
			{
				switch (type)
				{
				case MaterialValueType::Bool:
					if (!propertyJson.contains("value"))
						return false;
					outValue.Data = propertyJson.at("value").get<bool>();
					return true;

				case MaterialValueType::Int:
					if (!propertyJson.contains("value"))
						return false;
					outValue.Data = propertyJson.at("value").get<int32_t>();
					return true;

				case MaterialValueType::Float:
					if (!propertyJson.contains("value"))
						return false;
					outValue.Data = propertyJson.at("value").get<float>();
					return true;

				case MaterialValueType::Float2:
					if (!propertyJson.contains("value"))
						return false;
					outValue.Data = DeserializeVec2(propertyJson.at("value"));
					return true;

				case MaterialValueType::Float3:
					if (!propertyJson.contains("value"))
						return false;
					outValue.Data = JsonUtils::DeserializeVec3(propertyJson.at("value"));
					return true;

				case MaterialValueType::Float4:
				case MaterialValueType::Color:
					if (!propertyJson.contains("value"))
						return false;
					outValue.Data = JsonUtils::DeserializeVec4(propertyJson.at("value"));
					return true;

				case MaterialValueType::Texture2D:
				case MaterialValueType::TextureCube:
					if (propertyJson.contains("asset"))
					{
						outValue.Data = JsonUtils::DeserializeAssetHandle(propertyJson.at("asset"));
						return true;
					}
					if (propertyJson.contains("name") && propertyJson.at("name").is_string())
					{
						outValue.Data = propertyJson.at("name").get<std::string>();
						return true;
					}

					// 缺资源时保留空句柄，避免反序列化直接失败。
					outValue.Data = InvalidAssetHandle;
					return true;

				default:
					break;
				}
			}
			catch (const std::exception& e)
			{
				KITA_CORE_WARN(
					"Material property '{0}' deserialize failed: {1}, reason: {2}",
					propertyName,
					path.string(),
					e.what());
				return false;
			}

			return false;
		}

		static json SerializePropertyMap(const std::unordered_map<std::string, MaterialPropertyValue>& properties)
		{
			json result = json::object();

			for (const auto& [name, value] : properties)
			{
				result[name] = SerializePropertyValue(value);
			}

			return result;
		}

		static void DeserializePropertyMap(
			const json& propertyMapJson,
			std::unordered_map<std::string, MaterialPropertyValue>& outProperties,
			const std::filesystem::path& path,
			const char* sectionName)
		{
			outProperties.clear();

			if (!propertyMapJson.is_object())
			{
				KITA_CORE_WARN(
					"Material section '{0}' is not an object: {1}",
					sectionName,
					path.string());
				return;
			}

			for (auto it = propertyMapJson.begin(); it != propertyMapJson.end(); ++it)
			{
				MaterialPropertyValue value{};
				if (DeserializePropertyValue(it.value(), value, path, it.key()))
				{
					outProperties[it.key()] = std::move(value);
				}
			}
		}

		static bool DeserializeV3(const json& root, const std::filesystem::path& path, MaterialAsset& materialAsset)
		{
			if (root.contains("shaderLab"))
			{
				materialAsset.ShaderLabHandle = JsonUtils::DeserializeAssetHandle(root.at("shaderLab"));
			}

			// 兼容保留字段
			if (root.contains("shader"))
			{
				materialAsset.ShaderHandle = JsonUtils::DeserializeAssetHandle(root.at("shader"));
			}

			if (root.contains("properties"))
			{
				DeserializePropertyMap(
					root.at("properties"),
					materialAsset.PropertyBlock.Values,
					path,
					"properties");
			}

			if (root.contains("orphans"))
			{
				DeserializePropertyMap(
					root.at("orphans"),
					materialAsset.PropertyBlock.OrphanValues,
					path,
					"orphans");
			}

			return true;
		}

		static void AddLegacyScalarProperty(
			MaterialAsset& materialAsset,
			const std::string& propertyName,
			MaterialValueType type,
			const auto& value)
		{
			MaterialPropertyValue propertyValue{};
			propertyValue.ValueType = type;
			propertyValue.Data = value;
			materialAsset.PropertyBlock.Values[propertyName] = std::move(propertyValue);
		}

		static void AddLegacyTextureProperty(
			MaterialAsset& materialAsset,
			const std::string& propertyName,
			MaterialValueType type,
			AssetHandle handle)
		{
			MaterialPropertyValue propertyValue{};
			propertyValue.ValueType = type;
			propertyValue.Data = handle;
			materialAsset.PropertyBlock.Values[propertyName] = std::move(propertyValue);
		}

		static bool DeserializeLegacy(const json& root, const std::filesystem::path& path, MaterialAsset& materialAsset)
		{
			if (!root.contains("shader"))
			{
				KITA_CORE_WARN("Material file missing 'shader': {0}", path.string());
				materialAsset.ShaderHandle = InvalidAssetHandle;
			}
			else
			{
				materialAsset.ShaderHandle = JsonUtils::DeserializeAssetHandle(root.at("shader"));
			}

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
				// 更早期格式兼容。
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
				materialAsset.m_SurfaceParams.BaseColor = JsonUtils::DeserializeVec4(root.at("baseColor"));
			}

			// 旧版材质向新版 PropertyBlock 做一份映射。
			// 这是迁移阶段最关键的一步，后续即使运行时还没完全切换，
			// 编辑器与新材质系统也已经可以读取统一属性容器。
			AddLegacyScalarProperty(materialAsset, "_BaseColor", MaterialValueType::Color, materialAsset.m_SurfaceParams.BaseColor);
			AddLegacyScalarProperty(materialAsset, "_Emissive", MaterialValueType::Float3, materialAsset.m_SurfaceParams.Emissive);
			AddLegacyScalarProperty(materialAsset, "_Metallic", MaterialValueType::Float, materialAsset.m_SurfaceParams.Metallic);
			AddLegacyScalarProperty(materialAsset, "_Roughness", MaterialValueType::Float, materialAsset.m_SurfaceParams.Roughness);
			AddLegacyScalarProperty(materialAsset, "_AmbientOcclusion", MaterialValueType::Float, materialAsset.m_SurfaceParams.AmbientOcclusion);
			AddLegacyScalarProperty(materialAsset, "_Opacity", MaterialValueType::Float, materialAsset.m_SurfaceParams.Opacity);
			AddLegacyScalarProperty(materialAsset, "_NormalScale", MaterialValueType::Float, materialAsset.m_SurfaceParams.NormalScale);
			AddLegacyScalarProperty(materialAsset, "_AlphaCutoff", MaterialValueType::Float, materialAsset.m_SurfaceParams.AlphaCutoff);

			AddLegacyTextureProperty(materialAsset, "_Albedo", MaterialValueType::Texture2D, materialAsset.m_Textures.Albedo);
			AddLegacyTextureProperty(materialAsset, "_Normal", MaterialValueType::Texture2D, materialAsset.m_Textures.Normal);
			AddLegacyTextureProperty(materialAsset, "_MetallicRoughness", MaterialValueType::Texture2D, materialAsset.m_Textures.MetallicRoughness);
			AddLegacyTextureProperty(materialAsset, "_AmbientOcclusionTexture", MaterialValueType::Texture2D, materialAsset.m_Textures.AmbientOcclusion);
			AddLegacyTextureProperty(materialAsset, "_EmissiveTexture", MaterialValueType::Texture2D, materialAsset.m_Textures.Emissive);
			AddLegacyTextureProperty(materialAsset, "_OpacityTexture", MaterialValueType::Texture2D, materialAsset.m_Textures.Opacity);

			return true;
		}
	};

}
