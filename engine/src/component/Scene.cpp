#include "kita_pch.h"
#include "Scene.h"
#include "Object.h"
#include "Transform.h"
#include "MeshRenderer.h"
#include "LineRenderer.h"
#include "LightComponent.h"
#include "render/Renderer.h"
#include "render/RenderAssetUtils.h"
#include <render/scene/Gizmo.h>


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

	void Scene::LoadSkyCubemap(const CubemapFacePaths& faces)
	{
		TextureDescriptor desc{};
		desc.EnableMipMaps = true;
		m_SkyCubemap = Texture::CreateCubeMap(desc, faces);
	}

	void Scene::Clear()
	{
		m_Registry.clear();
	}

	void Scene::SimulateSceneEditor()
	{

		auto lineView = m_Registry.view<LineRenderer>();

		std::vector<GizmoPointUBOData> gizmoPoints = {};
		for (auto entity : lineView)
		{
			auto& lineRenderer = lineView.get<LineRenderer>(entity);
			lineRenderer.RebuildIfNeeded();
		}
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
			for (size_t i = 0;i < meshs.size();i++)
			{
				auto mesh = meshs[i];
				if (i >= mats.size() || !mats[i])
					continue;

				auto mat = mats[i];
				auto shader = mat->GetShader();
				if (!shader)
					continue;

				uint32_t id = (uint32_t)entity;
				shader->SetMat4("Model", model);
				shader->SetInt("id", id);
				mat->Bind();
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

			auto lineShader = RenderAssetUtils::GetRuntimeShader("packages/render/shaders/EditorLineShader.glsl");
			if (!lineShader)
			{
				continue;
			}
			lineShader->SetMat4("Model", model);
			lineShader->SetInt("id", id);
			lineShader->SetColor("Color", lineRenderer.GetLineColor());
			Renderer::SubmitAsLine(
				lineRenderer.GetCurveVAO(),
				lineShader,
				lineRenderer.GetCurveVertexCount(),
				lineRenderer.GetLineWidth());
			lineRenderer.RenderEditorHelpers(model, id);

			for (auto& point : points)
			{
				if (!lineRenderer.IsAnchorControlPoint(point.id))
					continue;

				auto gizmoPoint = GizmoPointUBOData{};
				gizmoPoint.position = point.position;
				gizmoPoint.color = point.color;
				gizmoPoint.radius = lineRenderer.GetControlPointRadius(point.id);
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

