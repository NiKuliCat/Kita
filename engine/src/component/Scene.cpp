#include "kita_pch.h"
#include "Scene.h"
#include "Object.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "LineRenderer.h"
#include "render/Renderer.h"
#include "render/ShaderLibrary.h"
#include <render/scene/Gizmo.h>
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

	void Scene::LoadSkyCubemap(const CubemapFacePaths& faces)
	{
		TextureDescriptor desc{};
		desc.EnableMipMaps = true;


		m_SkyCubemap = Texture::CreateCubeMap(desc, faces);
	}

	void Scene::RenderSceneEditor()  
	{
		// 渲染实体
		auto mesh = m_Registry.group<Transform, MeshRenderer>();

		for (auto entity : mesh)
		{
			auto [transform, meshRenderer] = mesh.get<Transform, MeshRenderer>(entity);

			glm::mat4 model = transform.GetTransformMatrix();

			auto shader = meshRenderer.GetMaterial()->GetShader();
			uint32_t id = (uint32_t)entity;
			shader->SetMat4("Model", model);
			shader->SetInt("id", id);

			auto& shaderLib = ShaderLibrary::GetInstance();
			auto lineShader = shaderLib.Get("EditorLineShader");
			const auto& meshs = meshRenderer.GetMeshs();
			lineShader->SetMat4("Model", model);
			lineShader->SetInt("id", id);

			for (const auto& mesh : meshs)
			{
				Renderer::Submit(mesh->GetVAO(), shader);
			}
		}


		//天空盒
		Renderer::DrawSkyBox(m_SkyCubemap, 9);


		//gizmo
		auto lineView = m_Registry.view<Transform, LineRenderer>();

		std::vector<GizmoPointUBOData> gizmoPoints = {};
		for (auto entity : lineView)
		{
			auto& transform = lineView.get<Transform>(entity);
			auto& lineRenderer = lineView.get<LineRenderer>(entity);
			glm::mat4 model = transform.GetTransformMatrix();
			auto points = lineRenderer.GetControlPoints();
			uint32_t id = (uint32_t)entity;


			for (auto point : points)
			{
				auto gizmoPoint = GizmoPointUBOData{};
				gizmoPoint.position = point.position;
				gizmoPoint.color = point.color;
				gizmoPoint.radius = 8.0f;
				gizmoPoint.index = point.id;
				gizmoPoints.push_back(gizmoPoint);
			}

			Gizmo::DrawPoints(gizmoPoints);
			Gizmo::FlushAllPoints(model,id);

			gizmoPoints.clear();
		}

		auto settings = EditorGridSettings{};
		settings.CellSize = 1.0f;
		Renderer::DrawEditorGrids(settings);

	}
}

