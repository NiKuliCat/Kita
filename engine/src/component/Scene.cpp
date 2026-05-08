#include "kita_pch.h"
#include "Scene.h"
#include "Object.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "LightComponent.h"
namespace Kita {

	Scene::~Scene()
	{
	}
	Object Scene::CreateObject(const std::string& name)
	{
		Object obj = { m_Registry.create(), this, name };
		KITA_CORE_DEBUG("the uuid of {0} is : {1} ", obj.GetName(), obj.GetUUID());
		return obj;
	}
	Object Scene::CreateObjectWithUUID(UUID uuid, const std::string& name)
	{
		return Object(m_Registry.create(), this, uuid, name);
	}
	void Scene::DestroyObject(Object object)
	{
		m_Registry.destroy(object);
	}


	void Scene::Clear()
	{
		m_Registry.clear();
	}

	void Scene::SimulateSceneEditor()
	{
	}

	void Scene::RenderSceneEditor()
	{
		// 渲染实体
		auto mesh = m_Registry.group<Transform, MeshRenderer>();

		for (auto entity : mesh)
		{
			auto [transform, meshRenderer] = mesh.get<Transform, MeshRenderer>(entity);

			glm::mat4 model = transform.GetTransformMatrix();


			const auto& meshs = meshRenderer.GetMeshs();
			const auto& mats = meshRenderer.GetRuntimeMaterials();
		}


		//gizmo
		

	}

	DirectLightData Scene::GetMainDirectLightData() const
	{
		DirectLightData lightData{};
		lightData.Color = glm::vec4(1.0f);
		lightData.Direction = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

		auto lightView = m_Registry.view<Transform, LightComponent>();
		for (auto entity : lightView)
		{
			auto [lightTransform, lightComponent] = lightView.get<Transform, LightComponent>(entity);
			lightData.Direction = glm::vec4(lightTransform.GetFrontDir(), lightComponent.intensity);
			lightData.Color = lightComponent.color;
			break;
		}

		return lightData;
	}
}

