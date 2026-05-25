#include "kita_pch.h"
#include "SceneSerializer.h"
#include "asset/AssetManager.h"

namespace Kita {



	SceneSerializer::SceneSerializer(const Ref<Scene>& scene)
		:m_Scene(scene)
	{
	}

	bool SceneSerializer::Serialize()
	{
		return Serialize(m_Scene->GetFilePath());
	}

	bool SceneSerializer::Serialize(const std::filesystem::path& filepath)
	{
		std::filesystem::path path(filepath);
		if (path.has_parent_path())
		{
			std::filesystem::create_directories(path.parent_path());
		}

		json root;
		root["version"] = 1;
		root["scene"]["name"] = m_Scene->GetName();
		const SceneRenderSettings& renderSettings = m_Scene->GetRenderSettings();
		root["scene"]["renderSettings"] = {
			{ "environmentSourceTexHandle", JsonUtils::SerializeAssetHandle(renderSettings.EnvironmentSourceTexHandle) },
			{ "sourceType", renderSettings.SourceType == EnvironmentSourceType::Cubemap ? "Cubemap" : "Equirectangular" },
			{ "skyboxTextureHandle", JsonUtils::SerializeAssetHandle(renderSettings.SkyboxTextureHandle) },
			{ "skyboxIntensity", renderSettings.SkyboxIntensity },
			{ "skyboxRotationY", renderSettings.SkyboxRotationY },
			{ "skyboxMipLevel", renderSettings.SkyboxMipLevel },
			{ "environmentCubeSize", renderSettings.EnvironmentCubeSize },
			{ "irradianceCubeSize", renderSettings.IrradianceCubeSize },
			{ "prefilterCubeSize", renderSettings.PrefilterCubeSize },
			{ "dfgLutSize", renderSettings.DfgLutSize },
			{ "intensity", renderSettings.Intensity },
			{ "rotationY", renderSettings.RotationY }
		};
		root["objects"] = json::array();
		auto view = m_Scene->GetRegistry().view<entt::entity>();
		for (auto entity : view)
		{
			Object object(entity, m_Scene.get());

			json objectJson;
			SerializeObject(objectJson, object);
			root["objects"].push_back(objectJson);
		}

		std::ofstream out(filepath);
		if (!out.is_open())
		{
			return false;
		}

		out << root.dump(4);
		m_Scene->SetFilePath(path);
		KITA_CLENT_DEBUG("{0} scene scuessfully saved to {1}", m_Scene->GetName(), filepath.string());
		return true;
	}

	Ref<Scene> SceneSerializer::Deserialize(const std::filesystem::path& filepath)
	{
		std::ifstream in(filepath);
		if (!in.is_open())
		{
			KITA_CORE_ERROR("Failed to open scene file: {0}", filepath.string());
			return nullptr;
		}
		json root;
		in >> root;

		KITA_CORE_ASSERT(root.is_object(), "Scene file root must be a json object.");
		KITA_CORE_ASSERT(root.contains("version"), "Scene file missing version.");
		KITA_CORE_ASSERT(root.at("version").get<uint32_t>() == 1, "version not is 1.0");
		KITA_CORE_ASSERT(root.contains("scene"), "Scene file missing scene node.");
		KITA_CORE_ASSERT(root.contains("objects"), "Scene file missing objects node.");

		m_Scene->Clear();


		if (root["scene"].contains("name"))
		{
			m_Scene->SetName(root["scene"]["name"].get<std::string>());
		}

		if (root["scene"].contains("renderSettings") && root["scene"]["renderSettings"].is_object())
		{
			const json& renderSettingsJson = root["scene"]["renderSettings"];
			SceneRenderSettings& renderSettings = m_Scene->GetRenderSettings();
			auto& assetManager = AssetManager::GetInstance();

			if (renderSettingsJson.contains("environmentSourceTexHandle"))
			{
				renderSettings.EnvironmentSourceTexHandle =
					JsonUtils::DeserializeAssetHandle(renderSettingsJson.at("environmentSourceTexHandle"));
				if (!Asset::IsValidHandle(renderSettings.EnvironmentSourceTexHandle) ||
					!assetManager.HasHandle(renderSettings.EnvironmentSourceTexHandle))
				{
					renderSettings.EnvironmentSourceTexHandle = InvalidAssetHandle;
				}
			}

			if (renderSettingsJson.contains("sourceType"))
			{
				const std::string sourceType = renderSettingsJson.at("sourceType").get<std::string>();
				renderSettings.SourceType =
					sourceType == "Cubemap" ? EnvironmentSourceType::Cubemap : EnvironmentSourceType::Equirectangular;
			}

			if (renderSettingsJson.contains("skyboxTextureHandle"))
			{
				renderSettings.SkyboxTextureHandle =
					JsonUtils::DeserializeAssetHandle(renderSettingsJson.at("skyboxTextureHandle"));
				if (!Asset::IsValidHandle(renderSettings.SkyboxTextureHandle) ||
					!assetManager.HasHandle(renderSettings.SkyboxTextureHandle))
				{
					renderSettings.SkyboxTextureHandle = InvalidAssetHandle;
				}
			}

			if (renderSettingsJson.contains("skyboxIntensity"))
				renderSettings.SkyboxIntensity = renderSettingsJson.at("skyboxIntensity").get<float>();
			if (renderSettingsJson.contains("skyboxRotationY"))
				renderSettings.SkyboxRotationY = renderSettingsJson.at("skyboxRotationY").get<float>();
			if (renderSettingsJson.contains("skyboxMipLevel"))
				renderSettings.SkyboxMipLevel = renderSettingsJson.at("skyboxMipLevel").get<float>();
			if (renderSettingsJson.contains("environmentCubeSize"))
				renderSettings.EnvironmentCubeSize = renderSettingsJson.at("environmentCubeSize").get<uint32_t>();
			if (renderSettingsJson.contains("irradianceCubeSize"))
				renderSettings.IrradianceCubeSize = renderSettingsJson.at("irradianceCubeSize").get<uint32_t>();
			if (renderSettingsJson.contains("prefilterCubeSize"))
				renderSettings.PrefilterCubeSize = renderSettingsJson.at("prefilterCubeSize").get<uint32_t>();
			if (renderSettingsJson.contains("dfgLutSize"))
				renderSettings.DfgLutSize = renderSettingsJson.at("dfgLutSize").get<uint32_t>();
			if (renderSettingsJson.contains("intensity"))
				renderSettings.Intensity = renderSettingsJson.at("intensity").get<float>();
			if (renderSettingsJson.contains("rotationY"))
				renderSettings.RotationY = renderSettingsJson.at("rotationY").get<float>();
		}

		const auto& objects = root["objects"];
		KITA_CORE_ASSERT(objects.is_array(), "Scene objects must be a json array.");

		for (const auto& objectData : objects)
		{
			DeserializeObject(objectData);
		}

		return m_Scene;
	}

	void SceneSerializer::SerializeObject(json& objectJson, Object object)
	{
		objectJson["uuid"] = JsonUtils::SerializeUUID(object.GetUUID());
		objectJson["name"] = object.GetName();
		objectJson["transform"] = JsonUtils::SerializeTransform(object.GetComponent<Transform>());

		if (object.HasComponent<MeshRenderer>())
		{
			objectJson["meshRenderer"] = ComponentSerializer::SerializeMeshRenderer(object.GetComponent<MeshRenderer>());
		}

		if (object.HasComponent<LightComponent>())
		{
			objectJson["lightComponent"] = ComponentSerializer::SerializeLightComponent(object.GetComponent<LightComponent>());
		}

	}

	void SceneSerializer::DeserializeObject(const json& entityData)
	{
		UUID uuid = JsonUtils::DeserializeUUID(entityData.at("uuid"));
		std::string name = entityData.at("name").get<std::string>();

		Object object = m_Scene->CreateObjectWithUUID(uuid, name);
		Transform transform = JsonUtils::DeserializeTransform(entityData.at("transform"));
		object.GetComponent<Transform>() = transform;

		if (entityData.contains("meshRenderer"))
		{
			auto& meshRenderer = object.AddComponent<MeshRenderer>();
			meshRenderer = ComponentSerializer::DeserializeMeshRenderer(entityData.at("meshRenderer"));
		}

		if (entityData.contains("lightComponent"))
		{
			auto& lightComponent = object.AddComponent<LightComponent>();
			lightComponent = ComponentSerializer::DeserializeLightComponent(entityData.at("lightComponent"));
		}

	}



	json ComponentSerializer::SerializeMeshRenderer(const MeshRenderer& meshRenderer)
	{
		json meshRendererJson;
		meshRendererJson["meshAsset"] = JsonUtils::SerializeAssetHandle(meshRenderer.MeshAssetHandle);

		meshRendererJson["defaultMaterialAsset"] = JsonUtils::SerializeAssetHandle(meshRenderer.DefaultMaterialAssetHandle);
		meshRendererJson["materialAssets"] = json::array();
		for (const AssetHandle materialHandle : meshRenderer.MaterialAssetHandles)
		{
			meshRendererJson["materialAssets"].push_back( JsonUtils::SerializeAssetHandle(materialHandle));
		}

		return meshRendererJson;
	}

	MeshRenderer ComponentSerializer::DeserializeMeshRenderer(const json& meshRendererJson)
	{
		MeshRenderer meshRenderer{};
		if (!meshRendererJson.is_object())
		{
			return meshRenderer;
		}

		auto& assetManager = AssetManager::GetInstance();

		if (meshRendererJson.contains("meshAsset"))
		{
			meshRenderer.MeshAssetHandle =
				JsonUtils::DeserializeAssetHandle(meshRendererJson.at("meshAsset"));

			if (!Asset::IsValidHandle(meshRenderer.MeshAssetHandle) ||
				!assetManager.HasHandle(meshRenderer.MeshAssetHandle))
			{
				meshRenderer.MeshAssetHandle = InvalidAssetHandle;
			}
		}

		if (meshRendererJson.contains("defaultMaterialAsset"))
		{
			meshRenderer.DefaultMaterialAssetHandle =
				JsonUtils::DeserializeAssetHandle(meshRendererJson.at("defaultMaterialAsset"));

			if (!Asset::IsValidHandle(meshRenderer.DefaultMaterialAssetHandle) ||
				!assetManager.HasHandle(meshRenderer.DefaultMaterialAssetHandle))
			{
				meshRenderer.DefaultMaterialAssetHandle = InvalidAssetHandle;
			}
		}

		if (meshRendererJson.contains("materialAssets") &&
			meshRendererJson.at("materialAssets").is_array())
		{
			for (const auto& item : meshRendererJson.at("materialAssets"))
			{
				AssetHandle materialHandle = JsonUtils::DeserializeAssetHandle(item);
				if (!Asset::IsValidHandle(materialHandle) || !assetManager.HasHandle(materialHandle))
				{
					materialHandle = InvalidAssetHandle;
				}

				meshRenderer.MaterialAssetHandles.push_back(materialHandle);
			}
		}

		return meshRenderer;
	}

	json ComponentSerializer::SerializeLightComponent(const LightComponent& lightComponent)
	{
		json lightComponentJson;
		lightComponentJson["color"] = JsonUtils::SerializeVec4(lightComponent.color);
		lightComponentJson["intensity"] = lightComponent.intensity;

		return lightComponentJson;
	}

	LightComponent ComponentSerializer::DeserializeLightComponent(const json& lightComponentJson)
	{
		LightComponent light;
		light.color = JsonUtils::DeserializeVec4(lightComponentJson.at("color"));
		light.intensity = lightComponentJson.at("intensity").get<float>();

		return light;
	}
}
