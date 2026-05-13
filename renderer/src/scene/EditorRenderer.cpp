#include "renderer_pch.h"
#include "EditorRenderer.h"

#include "asset/AssetManager.h"
#include "component/MeshRenderer.h"
#include "component/Transform.h"
#include "ui/viewport/EditorPickRegistry.h"
#include "ui/viewport/EditorViewportSurface.h"
#include "project/EditorProjectBootstrap.h"
namespace Kita {

	EditorRenderer::EditorRenderer(
		VulkanContext& context,
		VulkanRenderTarget& rt,
		VulkanRenderTarget& pickingRt,
		VulkanResourceFactory& vulkanResFactory,
		PipelineFactory& pipelineFactory,
		const Ref<Scene>& scene,
		ViewportCamera& camera,
		EditorPickRegistry& pickRegistry)
		: m_Context(&context)
		, m_VulkanResFactory(&vulkanResFactory)
		, m_RenderTarget(&rt)
		, m_SceneContext(scene)
		, m_PickRegistry(&pickRegistry)
		, m_ViewportCamera(&camera)
		, m_PipelineFactory(&pipelineFactory)
	{
		Init();
		m_SceneBindings.Init(context, context.GetFramesInFlight());
		m_ForwardOpaquePass = CreateUnique<ForwardOpaquePass>(m_SceneBindings, MakeForwardOpaquePassDesc(rt));
		m_ViewportPickingPass = CreateUnique<ViewportPickingPass>(m_SceneBindings,MakeViewportPickingPassDesc(pickingRt));
	}

	void EditorRenderer::Init()
	{

	}

	void EditorRenderer::OnDestroy()
	{

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

	VulkanGraphicsPipeline* EditorRenderer::GetPipeline(VulkanRenderTarget& rt, Ref<VulkanGeometry>& geometry, Ref<VulkanMaterial>& material)
	{
		PipelineRequest request{};
		request.Pass = PassType::ForwardOpaque;
		request.Geometry = geometry.get();
		request.VertexShader = material->GetVertexShader().get();
		request.FragmentShader = material->GetFragmentShader().get();
		request.ColorFormat = rt.GetColorFormat(0);
		request.DepthFormat = rt.GetDepthFormat();
		request.Samples = VK_SAMPLE_COUNT_1_BIT;

		request.DescriptorSetLayouts = {
			m_SceneBindings.GetDescriptorSet(0).GetLayout(),
			material->GetDescriptorSet(0).GetLayout()
		};

		request.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		request.PolygonMode = VK_POLYGON_MODE_FILL;
		request.CullMode = VK_CULL_MODE_NONE;
		request.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		request.EnableDepthTest = true;
		request.EnableDepthWrite = true;
		request.EnableBlending = false;

		request.PushConstantStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		request.PushConstantSize = ObjectDataSize;

		VulkanGraphicsPipeline* pipeline = m_PipelineFactory->GetOrCreate(request);

		return pipeline;
	}

	VulkanGraphicsPipeline* EditorRenderer::GetPickingPipeline(VulkanRenderTarget& rt, Ref<VulkanGeometry>& geometry)
	{
		if (!m_ViewportPickingPass)
			return nullptr;

		;
		VulkanResourceFactory::ShaderBundle shaderBundle = m_VulkanResFactory->GetOrCreateShaderBundle(EditorProjectBootstrap::GetPreLoadShaderHandle("picking"));

		PipelineRequest request{};
		request.Pass = PassType::EditorPicking;
		request.Geometry = geometry.get();
		request.VertexShader = shaderBundle.VertexShader.get();
		request.FragmentShader = shaderBundle.FragmentShader.get();
		request.ColorFormat = rt.GetColorFormat(0);
		request.DepthFormat = rt.HasDepthAttachment() ? rt.GetDepthFormat() : VK_FORMAT_UNDEFINED;
		request.Samples = VK_SAMPLE_COUNT_1_BIT;
		request.DescriptorSetLayouts = {
			m_SceneBindings.GetDescriptorSet(0).GetLayout()
		};
		request.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		request.PolygonMode = VK_POLYGON_MODE_FILL;
		request.CullMode = VK_CULL_MODE_NONE;
		request.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		request.EnableDepthTest = true;
		request.EnableDepthWrite = true;
		request.EnableBlending = false;
		request.PushConstantStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		request.PushConstantSize = ViewportPickingPushConstantSize;

		return m_PipelineFactory->GetOrCreate(request);
	}

	void EditorRenderer::Render(EditorViewportSurface& surface)
	{
		if (!m_Context || !m_SceneContext || !m_ViewportCamera || !m_PickRegistry)
			return;

		VulkanRenderTarget& rt = surface.GetRenderTarget();
		VulkanRenderTarget& pickingRt = surface.GetPickingRenderTarget();
		ScenePassData sceneData{};
		InitRenderSceneData(sceneData);

		m_ForwardOpaquePass->SetSceneData(sceneData);
		if (m_ViewportPickingPass)
		{
			ScenePassData pickingSceneData = sceneData;
			pickingSceneData.BeginInfo.ClearColor = glm::vec4(0.0f);
			pickingSceneData.BeginInfo.TransitionSampledColors = false;
			pickingSceneData.BeginInfo.TransitionSampledDepth = false;
			m_ViewportPickingPass->SetSceneData(pickingSceneData);
		}

		auto cmd = m_Context->GetCurrentCommandBuffer();
		if (cmd == VK_NULL_HANDLE)
			return;
		m_ForwardOpaquePass->ClearDrawItems();
		if (m_ViewportPickingPass)
			m_ViewportPickingPass->ClearDrawItems();

		auto mesh = m_SceneContext->GetRegistry().group<Transform, MeshRenderer>();
		for (auto entity : mesh)
		{
			auto [transform, meshRenderer] = mesh.get<Transform, MeshRenderer>(entity);
			Object object{ entity, m_SceneContext.get(), "" };
			if (!object)
				continue;

			glm::mat4 model = transform.GetTransformMatrix();
			ObjectData objectData{};
			objectData.Matrix_M = model;
			objectData.Matrix_I_M = glm::inverse(model);

			auto geometries = m_VulkanResFactory->GetOrCreateGeometries(meshRenderer.MeshAssetHandle);
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

				Ref<VulkanMaterial> material = m_VulkanResFactory->CreateMaterial(materialHandle);
				if (!material)
					continue;

				if (!material->GetAlbedoTexture())
				{
					KITA_CORE_WARN("EditorRenderer: material has no albedo texture, skipping draw.");
					continue;
				}

				VulkanGraphicsPipeline* pipeline = GetPipeline(rt, geometry, material);
				if (!pipeline)
					continue;

				ForwardOpaqueDrawItem item{};
				item.Pipeline = pipeline;
				item.Geometry = geometry.get();
				item.Material = material.get();
				item.PerObject = objectData;

				m_ForwardOpaquePass->AddDrawItem(item);

				if (m_ViewportPickingPass)
				{
					VulkanGraphicsPipeline* pickingPipeline = GetPickingPipeline(pickingRt, geometry);
					if (!pickingPipeline)
						continue;

					ViewportPickingDrawItem pickingItem{};
					pickingItem.Pipeline = pickingPipeline;
					pickingItem.Geometry = geometry.get();
					pickingItem.PushConstants.PerObject = objectData;
					pickingItem.PushConstants.PickID = m_PickRegistry->RegisterSceneObject(object);
					m_ViewportPickingPass->AddDrawItem(pickingItem);
				}
			}
		}

		RenderPassContext passContext(*m_Context, cmd, rt);
		RenderPassContext pickingPassContext(*m_Context, cmd, pickingRt);

		m_ForwardOpaquePass->Execute(passContext);
		if (m_ViewportPickingPass)
		{
			m_ViewportPickingPass->Execute(pickingPassContext);
		}
	}

}
