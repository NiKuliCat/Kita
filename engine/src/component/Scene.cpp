#include "kita_pch.h"
#include "Scene.h"
#include "Object.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "render/Renderer.h"
namespace Kita {

	Scene::~Scene()
	{
	}
	Object Scene::CreateObject(const std::string& name)
	{
		Object obj = { m_Registry.create(), this, name };

		return obj;
	}
	void Scene::DestroyObject(Object object)
	{
		m_Registry.destroy(object);
	}
	void Scene::OnUpdate(float deltaTime)
	{
		RenderSceneEditor();
	}
	void Scene::RenderSceneEditor()
	{
		auto view = m_Registry.group<Transform, MeshRenderer>();

		for (auto entity : view)
		{
			auto [transform, meshRenderer] = view.get<Transform, MeshRenderer>(entity);

			glm::mat4 model = transform.GetTransformMatrix();

			auto shader = meshRenderer.GetMaterial()->GetShader();
			shader->SetMat4("Model", model);

			const auto& meshs = meshRenderer.GetMeshs();
			for (const auto& mesh : meshs)
			{
				Renderer::Submit(mesh->GetVAO(), shader);
			}
		}
	}
}

