#pragma once
#include "component/Scene.h"
#include "component/Object.h"
#include "JsonUtils.h"

namespace Kita {

	class SceneSerializer
	{
	public:
		SceneSerializer() = default;
		SceneSerializer(const  Ref<Scene>& scene);

		bool Serialize();
		bool Serialize(const std::filesystem::path& filepath);

		Ref<Scene> Deserialize(const std::filesystem::path& filepath);

	private:
		void SerializeObject(json& entities, Object object);
		void DeserializeObject(const json& entityData);

	private:
		Ref<Scene> m_Scene;
	};
}