#include "renderer_pch.h"
#include "EditorRenderer.h"

#include "asset/AssetManager.h"
#include "component/MeshRenderer.h"
#include "component/Transform.h"

namespace Kita {

	EditorRenderer::EditorRenderer(VulkanContext& context, VulkanRenderTarget& rt, const Ref<Scene>& scene, ViewportCamera& camera)
		: m_Context(&context)
		, m_Renderer(context)
		, m_VulkanResFactory(context, AssetManager::GetInstance())
		, m_RenderTarget(&rt)
		, m_SceneContext(scene)
		, m_ViewportCamera(&camera)
	{
		Init();
	}

	void EditorRenderer::Init()
	{
	}

	void EditorRenderer::OnDestroy()
	{
		m_OpaquePipeline.reset();
		m_VulkanResFactory.Clear();
		m_Renderer.OnDestroy();
	}

	void EditorRenderer::InitRenderSceneData(VulkanRenderer::CameraUBO& cameraData, VulkanRenderer::DirectionLightUBO& mainLightData)
	{
		cameraData.Matrix_V = m_ViewportCamera->GetViewMatrix();
		cameraData.Matrix_P = m_ViewportCamera->GetProjectionMatrix();
		cameraData.Matrix_VP = m_ViewportCamera->GetViewProjectionMatrix();
		cameraData.Matrix_I_V = glm::inverse(cameraData.Matrix_V);
		cameraData.Matrix_I_P = glm::inverse(cameraData.Matrix_P);
		cameraData.Matrix_I_VP = glm::inverse(cameraData.Matrix_VP);
		cameraData.CameraPosWS = glm::vec4(m_ViewportCamera->GetPosition(), 1.0f);

		auto mainlight = m_SceneContext->GetMainDirectLightData();
		mainLightData.Color = mainlight.Color;
		mainLightData.Direction = mainlight.Direction;
	}

	void EditorRenderer::EnsureDefaultPipeline(VulkanRenderTarget& rt, VulkanGeometry& geometry, VulkanMaterial& material)
	{
		if (m_OpaquePipeline)
			return;

		if (!material.GetVertexShader() || !material.GetFragmentShader())
		{
			KITA_CORE_WARN("EditorRenderer: material shader is invalid, cannot create pipeline.");
			return;
		}

		VulkanGraphicsPipeline::CreateInfo pipelineInfo{};
		pipelineInfo.Name = "EditorRenderer_OpaquePipeline";
		pipelineInfo.VertexShader = material.GetVertexShader().get();
		pipelineInfo.FragmentShader = material.GetFragmentShader().get();
		pipelineInfo.Geometry = &geometry;
		pipelineInfo.ColorFormat = rt.GetColorFormat(0);
		pipelineInfo.DepthFormat = rt.GetDepthFormat();
		pipelineInfo.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineInfo.CullMode = VK_CULL_MODE_NONE;
		pipelineInfo.EnableDepthTest = true;
		pipelineInfo.EnableDepthWrite = true;
		pipelineInfo.EnableBlending = false;
		pipelineInfo.DescriptorSetLayouts = {
			m_Renderer.GetSceneUniformDescriptorSet().GetLayout(),
			material.GetDescriptorSet(0).GetLayout()
		};

		m_OpaquePipeline = CreateUnique<VulkanGraphicsPipeline>(*m_Context, pipelineInfo);
	}

	void EditorRenderer::Render(VulkanRenderTarget& rt)
	{
		if (!m_Context || !m_SceneContext || !m_ViewportCamera)
			return;

		VulkanRenderer::CameraUBO cameraData{};
		VulkanRenderer::DirectionLightUBO mainLightData{};
		const glm::vec4 clearColor(0.03f, 0.032f, 0.034f, 1.0f);
		InitRenderSceneData(cameraData, mainLightData);

		auto cmd = m_Context->GetCurrentCommandBuffer();
		if (cmd == VK_NULL_HANDLE)
			return;

		m_Renderer.BeginScene(cmd, rt, cameraData, mainLightData, clearColor);

		auto mesh = m_SceneContext->GetRegistry().group<Transform, MeshRenderer>();

		for (auto entity : mesh)
		{
			auto [transform, meshRenderer] = mesh.get<Transform, MeshRenderer>(entity);

			glm::mat4 model = transform.GetTransformMatrix();
			VulkanRenderer::ObjectData objectData{};
			objectData.Matrix_M = model;
			objectData.Matrix_I_M = glm::inverse(model);

			auto geometries = m_VulkanResFactory.GetOrCreateGeometries(meshRenderer.MeshAssetHandle);
			if (geometries.empty())
				continue;

			for (size_t i = 0; i < geometries.size(); ++i)
			{
				Ref<VulkanGeometry> geometry = geometries[i];
				if (!geometry)
					continue;

				AssetHandle materialHandle = meshRenderer.DefaultMaterialAssetHandle;
				if (i < meshRenderer.MaterialAssetHandles.size() &&
					Asset::IsValidHandle(meshRenderer.MaterialAssetHandles[i]))
				{
					materialHandle = meshRenderer.MaterialAssetHandles[i];
				}

				Ref<VulkanMaterial> material = m_VulkanResFactory.CreateMaterial(materialHandle);
				if (!material)
					continue;

				if (!material->GetAlbedoTexture())
				{
					KITA_CORE_WARN("EditorRenderer: material has no albedo texture, skipping draw.");
					continue;
				}

				EnsureDefaultPipeline(rt, *geometry, *material);
				if (!m_OpaquePipeline)
					continue;

				material->GetDescriptorSet(m_Context->GetCurrentFrameIndex()).Bind(cmd, m_OpaquePipeline->GetLayout(), 1);
				m_Renderer.SubmitMesh(cmd, *m_OpaquePipeline, *geometry, objectData);
			}
		}

		m_Renderer.EndScene(cmd, rt);
	}

}
