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
			return false;
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
		meshRendererJson["meshAsset"] =
			JsonUtils::SerializeAssetHandle(meshRenderer.GetMeshAssetHandle());

		meshRendererJson["materialAssets"] = json::array();
		const auto& materialHandles = meshRenderer.GetMaterialAssetHandles();
		for (const AssetHandle materialHandle : materialHandles)
		{
			meshRendererJson["materialAssets"].push_back(
				JsonUtils::SerializeAssetHandle(materialHandle));
		}

		return meshRendererJson;
	}

	MeshRenderer ComponentSerializer::DeserializeMeshRenderer(const json& meshRendererJson)
	{
		MeshRenderer meshRenderer;
		if (!meshRendererJson.is_object())
		{
			return meshRenderer;
		}

		auto& assetManager = AssetManager::GetInstance();

		AssetHandle meshAssetHandle = InvalidAssetHandle;
		if (meshRendererJson.contains("meshAsset"))
		{
			meshAssetHandle = JsonUtils::DeserializeAssetHandle(meshRendererJson.at("meshAsset"));
		}

		if (!Asset::IsValidHandle(meshAssetHandle))
		{
			return meshRenderer;
		}

		const AssetMetadata* meshMetadata = assetManager.GetMetadata(meshAssetHandle);
		if (!meshMetadata)
		{
			KITA_CORE_WARN("DeserializeMeshRenderer failed, mesh asset metadata not found: {}", meshAssetHandle);
			return meshRenderer;
		}

		meshRenderer.LoadMeshs(meshMetadata->relativePath.generic_string());

		if (!meshRendererJson.contains("materialAssets") || !meshRendererJson.at("materialAssets").is_array())
		{
			return meshRenderer;
		}

		const auto& materialArray = meshRendererJson.at("materialAssets");
		auto& materialHandles = meshRenderer.GetMaterialAssetHandles();

		const size_t count = std::min(materialHandles.size(), materialArray.size());
		for (size_t i = 0; i < count; ++i)
		{
			AssetHandle materialHandle = JsonUtils::DeserializeAssetHandle(materialArray[i]);
			if (!Asset::IsValidHandle(materialHandle) || !assetManager.HasHandle(materialHandle))
			{
				materialHandle = InvalidAssetHandle;
			}

			meshRenderer.SetMaterialAssetHandle(i, materialHandle);
			meshRenderer.SyncMaterial(i);
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
