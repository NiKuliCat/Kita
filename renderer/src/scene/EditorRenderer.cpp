#include "renderer_pch.h"
#include "EditorRenderer.h"

#include "asset/AssetManager.h"
#include "component/MeshRenderer.h"
#include "component/Transform.h"

namespace Kita {

	EditorRenderer::EditorRenderer(VulkanContext& context, VulkanRenderTarget& rt, const Ref<Scene>& scene, ViewportCamera& camera)
		: m_Context(&context)
		, m_VulkanResFactory(context, AssetManager::GetInstance())
		, m_RenderTarget(&rt)
		, m_SceneContext(scene)
		, m_ViewportCamera(&camera)
	{
		Init();
		m_SceneBindings.Init(context, context.GetFramesInFlight());
		m_ForwardOpaquePass = CreateUnique<ForwardOpaquePass>(m_SceneBindings, MakeForwardOpaquePassDesc(rt));
	}

	void EditorRenderer::Init()
	{

	}

	void EditorRenderer::OnDestroy()
	{
		m_OpaquePipeline.reset();
		m_VulkanResFactory.Clear();
	}

	void EditorRenderer::InitRenderSceneData(ScenePassData& sceneData)
	{
		sceneData.Camera.Matrix_V = m_ViewportCamera->GetViewMatrix();
		sceneData.Camera.Matrix_P = m_ViewportCamera->GetProjectionMatrix();
		sceneData.Camera.Matrix_VP = m_ViewportCamera->GetViewProjectionMatrix();
		sceneData.Camera.Matrix_I_V = glm::inverse(sceneData.Camera.Matrix_V);
		sceneData.Camera.Matrix_I_P = glm::inverse(sceneData.Camera.Matrix_P);
		sceneData.Camera.Matrix_I_VP = glm::inverse(sceneData.Camera.Matrix_VP);
		sceneData.Camera.CameraPosWS = glm::vec4(m_ViewportCamera->GetPosition(), 1.0f);

		auto mainlight = m_SceneContext->GetMainDirectLightData();
		sceneData.MainLight.Color = mainlight.Color;
		sceneData.MainLight.Direction = mainlight.Direction;

		sceneData.BeginInfo.ClearColor = glm::vec4(0.03f, 0.032f, 0.034f, 1.0f);
		sceneData.BeginInfo.ClearDepth = 1.0f;
		sceneData.BeginInfo.ClearStencil = 0;
		sceneData.BeginInfo.TransitionSampledColors = true;
		sceneData.BeginInfo.TransitionSampledDepth = false;
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
			m_SceneBindings.GetDescriptorSet(0).GetLayout(),
			material.GetDescriptorSet(0).GetLayout()
		};

		m_OpaquePipeline = CreateUnique<VulkanGraphicsPipeline>(*m_Context, pipelineInfo);
	}

	void EditorRenderer::Render(VulkanRenderTarget& rt)
	{
		if (!m_Context || !m_SceneContext || !m_ViewportCamera)
			return;

		ScenePassData sceneData{};
		InitRenderSceneData(sceneData);

		m_ForwardOpaquePass->SetSceneData(sceneData);

		auto cmd = m_Context->GetCurrentCommandBuffer();
		if (cmd == VK_NULL_HANDLE)
			return;
		m_ForwardOpaquePass->ClearDrawItems();

		auto mesh = m_SceneContext->GetRegistry().group<Transform, MeshRenderer>();
		for (auto entity : mesh)
		{
			auto [transform, meshRenderer] = mesh.get<Transform, MeshRenderer>(entity);

			glm::mat4 model = transform.GetTransformMatrix();
			ObjectData objectData{};
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

				ForwardOpaqueDrawItem item{};
				item.Pipeline = m_OpaquePipeline.get();
				item.Geometry = geometry.get();
				item.Material = material.get();
				item.PerObject = objectData;

				m_ForwardOpaquePass->AddDrawItem(item);
			}
		}

		RenderPassContext passContext(*m_Context, cmd, rt);

		m_ForwardOpaquePass->Execute(passContext);
	}

}
