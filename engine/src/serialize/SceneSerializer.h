#pragma once
#include "component/Scene.h"
#include "component/Object.h"
#include "JsonUtils.h"
#include "component/MeshRenderer.h"
#include "component/LightComponent.h"
#include "component/LineRenderer.h"
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


	class ComponentSerializer
	{
	public:
		static json  SerializeMeshRenderer(const MeshRenderer& meshRenderer);
		static MeshRenderer  DeserializeMeshRenderer(const json& meshRendererJson);

		static json  SerializeLightComponent(const LightComponent& lightComponent);
		static LightComponent  DeserializeLightComponent(const json& lightComponentJson);

		static json  SerializeLineRenderer(const LineRenderer& lineRenderer);
		static LineRenderer  DeserializeLineRenderer(const json& lineRendererJson);
	};
}