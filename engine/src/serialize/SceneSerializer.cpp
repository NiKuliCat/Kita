#include "kita_pch.h"
#include "SceneSerializer.h"

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
	}

	void SceneSerializer::DeserializeObject(const json& entityData)
	{
		UUID uuid = JsonUtils::DeserializeUUID(entityData.at("uuid"));
		std::string name = entityData.at("name").get<std::string>();

		Object object = m_Scene->CreateObjectWithUUID(uuid, name);
		Transform transform = JsonUtils::DeserializeTransform(entityData.at("transform"));
		object.GetComponent<Transform>() = transform;
	}

}