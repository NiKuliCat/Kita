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
		VulkanRenderTarget& gbufferRt,
		VulkanRenderTarget& finalRt,
		VulkanRenderTarget& pickingRt,
		VulkanResourceFactory& vulkanResFactory,
		PipelineFactory& pipelineFactory,
		const Ref<Scene>& scene,
		ViewportCamera& camera,
		EditorPickRegistry& pickRegistry)
		: m_Context(&context)
		, m_VulkanResFactory(&vulkanResFactory)
		, m_GBufferRenderTarget(&gbufferRt)
		, m_RenderTarget(&finalRt)
		, m_SceneContext(scene)
		, m_PickRegistry(&pickRegistry)
		, m_ViewportCamera(&camera)
		, m_PipelineFactory(&pipelineFactory)
	{
		Init();
		m_SceneBindings.Init(context, context.GetFramesInFlight());
		m_BasePass = CreateUnique<BasePass>(m_SceneBindings, MakeBasePassDesc(gbufferRt));
		m_DeferredLightingPass = CreateUnique<DeferredLightingPass>(m_SceneBindings, MakeDeferredLightingPassDesc(finalRt));
		m_DeferredLightingPass->Init(context, context.GetFramesInFlight());
		m_EditorGridPass = CreateUnique<EditorGridPass>(m_SceneBindings, MakeEditorGridPassDesc(finalRt));
		m_ViewportPickingPass = CreateUnique<ViewportPickingPass>(m_SceneBindings,MakeViewportPickingPassDesc(pickingRt));
		m_SkyboxPass = CreateUnique<SkyboxPass>(m_SceneBindings, MakeSkyboxPassDesc(finalRt));
		InitGridResources();
		InitDeferredLightingResources();
	}

	void EditorRenderer::Init()
	{
		const AssetHandle skyboxMaterialHandle = EditorProjectBootstrap::GetPreLoadMaterialHandle("skybox");
		if (Asset::IsValidHandle(skyboxMaterialHandle))
		{
			m_SkyboxMaterial = m_VulkanResFactory->CreateMaterial(skyboxMaterialHandle);
		}
	}

	void EditorRenderer::OnDestroy()
	{
		m_GridVertexShader.reset();
		m_GridFragmentShader.reset();
		m_DeferredLightingVertexShader.reset();
		m_DeferredLightingFragmentShader.reset();
		if (m_DeferredLightingPass)
			m_DeferredLightingPass->Destroy();
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

	void EditorRenderer::InitGridResources()
	{
		if (!m_Context || !m_EditorGridPass)
			return;

		const AssetHandle gridShaderHandle = EditorProjectBootstrap::GetPreLoadShaderHandle("grid");
		if (!Asset::IsValidHandle(gridShaderHandle))
		{
			KITA_CORE_WARN("EditorRenderer: preload shader handle 'grid' is invalid.");
			return;
		}

		VulkanResourceFactory::ShaderBundle shaderBundle = m_VulkanResFactory->GetOrCreateShaderBundle(gridShaderHandle);
		if (!shaderBundle.IsValid())
		{
			KITA_CORE_WARN("EditorRenderer: failed to create shader bundle for preload shader 'grid'.");
			return;
		}

		m_GridVertexShader = shaderBundle.VertexShader;
		m_GridFragmentShader = shaderBundle.FragmentShader;
	}

	void EditorRenderer::InitDeferredLightingResources()
	{
		if (!m_Context || !m_DeferredLightingPass)
			return;

		const AssetHandle deferredLightingShaderHandle = EditorProjectBootstrap::GetPreLoadShaderHandle("deferredLighting");
		if (!Asset::IsValidHandle(deferredLightingShaderHandle))
		{
			KITA_CORE_WARN("EditorRenderer: preload shader handle 'deferredLighting' is invalid.");
			return;
		}

		VulkanResourceFactory::ShaderBundle shaderBundle = m_VulkanResFactory->GetOrCreateShaderBundle(deferredLightingShaderHandle);
		if (!shaderBundle.IsValid())
		{
			KITA_CORE_WARN("EditorRenderer: failed to create shader bundle for preload shader 'deferredLighting'.");
			return;
		}

		m_DeferredLightingVertexShader = shaderBundle.VertexShader;
		m_DeferredLightingFragmentShader = shaderBundle.FragmentShader;
	}

	VulkanGraphicsPipeline* EditorRenderer::GetPipeline(VulkanRenderTarget& rt, Ref<VulkanGeometry>& geometry, Ref<VulkanMaterial>& material)
	{
		PipelineRequest request{};
		request.Pass = PassType::GBuffer;
		request.Geometry = geometry.get();
		request.VertexShader = material->GetVertexShader().get();
		request.FragmentShader = material->GetFragmentShader().get();

		request.ColorFormats.clear();
		for (uint32_t i = 0; i < rt.GetColorAttachmentCount(); ++i)
			request.ColorFormats.push_back(rt.GetColorFormat(i));

		request.DepthFormat = rt.GetDepthFormat();
		request.Samples = rt.GetCreateInfo().Samples;

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
		request.DepthCompareOp = VK_COMPARE_OP_LESS;
		request.EnableBlending = false;

		request.PushConstantStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		request.PushConstantSize = ObjectDataSize;

		VulkanGraphicsPipeline* pipeline = m_PipelineFactory->GetOrCreate(request);

		return pipeline;
	}

	VulkanGraphicsPipeline* EditorRenderer::GetDeferredLightingPipeline(VulkanRenderTarget& rt)
	{
		if (!m_DeferredLightingVertexShader || !m_DeferredLightingFragmentShader || !m_DeferredLightingPass)
			return nullptr;

		PipelineRequest request{};
		request.Pass = PassType::DeferredLighting;
		request.UseVertexInput = false;
		request.VertexShader = m_DeferredLightingVertexShader.get();
		request.FragmentShader = m_DeferredLightingFragmentShader.get();

		request.ColorFormats.clear();
		for (uint32_t i = 0; i < rt.GetColorAttachmentCount(); ++i)
			request.ColorFormats.push_back(rt.GetColorFormat(i));

		request.DepthFormat = rt.HasDepthAttachment() ? rt.GetDepthFormat() : VK_FORMAT_UNDEFINED;
		request.Samples = rt.GetCreateInfo().Samples;
		request.DescriptorSetLayouts = {
			m_SceneBindings.GetDescriptorSet(0).GetLayout(),
			m_DeferredLightingPass->GetDescriptorSet(0).GetLayout()
		};
		request.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		request.PolygonMode = VK_POLYGON_MODE_FILL;
		request.CullMode = VK_CULL_MODE_NONE;
		request.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		request.EnableDepthTest = true;
		request.EnableDepthWrite = true;
		request.DepthCompareOp = VK_COMPARE_OP_ALWAYS;
		request.EnableBlending = false;
		request.PushConstantStages = 0;
		request.PushConstantSize = 0;

		return m_PipelineFactory->GetOrCreate(request);
	}

	VulkanGraphicsPipeline* EditorRenderer::GetGridPipeline(VulkanRenderTarget& rt)
	{
		if (!m_GridVertexShader || !m_GridFragmentShader)
			return nullptr;

		PipelineRequest request{};
		request.Pass = PassType::PostProcess;
		request.UseVertexInput = false;
		request.VertexShader = m_GridVertexShader.get();
		request.FragmentShader = m_GridFragmentShader.get();

		request.ColorFormats.clear();
		for (uint32_t i = 0; i < rt.GetColorAttachmentCount(); ++i)
			request.ColorFormats.push_back(rt.GetColorFormat(i));

		request.DepthFormat = rt.HasDepthAttachment() ? rt.GetDepthFormat() : VK_FORMAT_UNDEFINED;
		request.Samples = rt.GetCreateInfo().Samples;
		request.DescriptorSetLayouts = {
			m_SceneBindings.GetDescriptorSet(0).GetLayout()
		};
		request.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		request.PolygonMode = VK_POLYGON_MODE_FILL;
		request.CullMode = VK_CULL_MODE_NONE;
		request.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		request.EnableDepthTest = rt.HasDepthAttachment();
		request.EnableDepthWrite = false;
		request.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		request.EnableBlending = true;
		request.PushConstantStages = VK_SHADER_STAGE_FRAGMENT_BIT;
		request.PushConstantSize = EditorGridPass::PushConstantSize;

		return m_PipelineFactory->GetOrCreate(request);
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

		request.ColorFormats.clear();
		for (uint32_t i = 0; i < rt.GetColorAttachmentCount(); ++i)
			request.ColorFormats.push_back(rt.GetColorFormat(i));
		request.DepthFormat = rt.HasDepthAttachment() ? rt.GetDepthFormat() : VK_FORMAT_UNDEFINED;
		request.Samples = rt.GetCreateInfo().Samples;
		request.DescriptorSetLayouts = {
			m_SceneBindings.GetDescriptorSet(0).GetLayout()
		};
		request.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		request.PolygonMode = VK_POLYGON_MODE_FILL;
		request.CullMode = VK_CULL_MODE_NONE;
		request.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		request.EnableDepthTest = true;
		request.EnableDepthWrite = true;
		request.DepthCompareOp = VK_COMPARE_OP_LESS;
		request.EnableBlending = false;
		request.PushConstantStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		request.PushConstantSize = ViewportPickingPushConstantSize;

		return m_PipelineFactory->GetOrCreate(request);
	}

	VulkanGraphicsPipeline* EditorRenderer::GetSkyboxPipeline(VulkanRenderTarget& rt)
	{
		if (!m_SkyboxMaterial)
		{
			return nullptr;
		}

		if (!m_SkyboxMaterial->GetVertexShader() || !m_SkyboxMaterial->GetFragmentShader())
		{
			return nullptr;
		}

		PipelineRequest request{};
		request.Pass = PassType::PostProcess;
		request.UseVertexInput = false;
		request.VertexShader = m_SkyboxMaterial->GetVertexShader().get();
		request.FragmentShader = m_SkyboxMaterial->GetFragmentShader().get();

		request.ColorFormats.clear();
		for (uint32_t i = 0; i < rt.GetColorAttachmentCount(); ++i)
			request.ColorFormats.push_back(rt.GetColorFormat(i));
		request.DepthFormat = rt.HasDepthAttachment() ? rt.GetDepthFormat() : VK_FORMAT_UNDEFINED;
		request.Samples = rt.GetCreateInfo().Samples;

		request.DescriptorSetLayouts = {
			m_SceneBindings.GetDescriptorSet(0).GetLayout(),
			m_SkyboxMaterial->GetDescriptorSet(0).GetLayout()
		};

		request.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		request.PolygonMode = VK_POLYGON_MODE_FILL;
		request.CullMode = VK_CULL_MODE_NONE;
		request.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		request.EnableDepthTest = true;
		request.EnableDepthWrite = false;
		request.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		request.EnableBlending = false;

		request.PushConstantStages = 0;
		request.PushConstantSize = 0;

		return m_PipelineFactory->GetOrCreate(request);
	}

	void EditorRenderer::Render(EditorViewportSurface& surface)
	{
		if (!m_Context || !m_SceneContext || !m_ViewportCamera || !m_PickRegistry)
			return;

		VulkanRenderTarget& gbufferRt = surface.GetGBufferRenderTarget();
		VulkanRenderTarget& finalRt = surface.GetFinalRenderTarget();
		VulkanRenderTarget& pickingRt = surface.GetPickingRenderTarget();
		ScenePassData sceneData{};
		InitRenderSceneData(sceneData);

		ScenePassData baseSceneData = sceneData;
		baseSceneData.BeginInfo.TransitionSampledDepth = true;
		m_BasePass->SetSceneData(baseSceneData);
		if (m_DeferredLightingPass)
		{
			ScenePassData lightingSceneData = sceneData;
			lightingSceneData.BeginInfo.ClearColors = true;
			lightingSceneData.BeginInfo.ClearDepthAttachment = true;
			lightingSceneData.BeginInfo.TransitionSampledColors = true;
			lightingSceneData.BeginInfo.TransitionSampledDepth = false;
			m_DeferredLightingPass->SetSceneData(lightingSceneData);
			m_DeferredLightingPass->SetGBufferInput(&gbufferRt);
		}
		if (m_EditorGridPass)
		{
			ScenePassData gridSceneData = sceneData;
			gridSceneData.BeginInfo.ClearColors = false;
			gridSceneData.BeginInfo.ClearDepthAttachment = false;
			gridSceneData.BeginInfo.TransitionSampledColors = true;
			gridSceneData.BeginInfo.TransitionSampledDepth = false;
			m_EditorGridPass->SetSceneData(gridSceneData);
		}

		if (m_SkyboxPass)
		{
			ScenePassData skyboxSceneData = sceneData;
			skyboxSceneData.BeginInfo.ClearColors = false;
			skyboxSceneData.BeginInfo.ClearDepthAttachment = false;
			skyboxSceneData.BeginInfo.TransitionSampledColors = true;
			skyboxSceneData.BeginInfo.TransitionSampledDepth = false;
			m_SkyboxPass->SetSceneData(skyboxSceneData);
		}


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
		m_BasePass->ClearDrawItems();
		if (m_ViewportPickingPass)
			m_ViewportPickingPass->ClearDrawItems();

		uint32_t meshEntityCount = 0;
		uint32_t drawItemCount = 0;
		uint32_t skippedEmptyGeometryCount = 0;
		uint32_t skippedNullGeometryCount = 0;
		uint32_t skippedInvalidMaterialCount = 0;
		uint32_t skippedInvalidShaderCount = 0;
		uint32_t skippedInvalidPipelineCount = 0;
		uint32_t skippedInvalidPickingPipelineCount = 0;

		auto mesh = m_SceneContext->GetRegistry().group<Transform, MeshRenderer>();
		for (auto entity : mesh)
		{
			++meshEntityCount;
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
			{
				++skippedEmptyGeometryCount;
				KITA_CORE_WARN(
					"EditorRenderer: object '{}' skipped, mesh handle {} produced no geometries. defaultMaterial={}, materialSlotCount={}",
					object.GetName(),
					meshRenderer.MeshAssetHandle,
					meshRenderer.DefaultMaterialAssetHandle,
					static_cast<uint32_t>(meshRenderer.MaterialAssetHandles.size()));
				continue;
			}

			for (size_t i = 0; i < geometries.size(); ++i)
			{
				Ref<VulkanGeometry> geometry = geometries[i];
				if (!geometry)
				{
					++skippedNullGeometryCount;
					continue;
				}

				AssetHandle materialHandle = meshRenderer.DefaultMaterialAssetHandle;
				if (i < meshRenderer.MaterialAssetHandles.size() &&
					Asset::IsValidHandle(meshRenderer.MaterialAssetHandles[i]))
				{
					materialHandle = meshRenderer.MaterialAssetHandles[i];
				}

				Ref<VulkanMaterial> material = m_VulkanResFactory->CreateMaterial(materialHandle);
				if (!material)
				{
					++skippedInvalidMaterialCount;
					KITA_CORE_WARN(
						"EditorRenderer: object '{}' skipped, material handle {} failed to create.",
						object.GetName(),
						materialHandle);
					continue;
				}

				if (!material->GetVertexShader() || !material->GetFragmentShader())
				{
					++skippedInvalidShaderCount;
					KITA_CORE_WARN(
						"EditorRenderer: object '{}' skipped, material handle {} has invalid shaders. VS='{}' FS='{}'",
						object.GetName(),
						materialHandle,
						material->GetVertexShader() ? material->GetVertexShader()->GetName() : std::string("<null>"),
						material->GetFragmentShader() ? material->GetFragmentShader()->GetName() : std::string("<null>"));
					continue;
				}

				m_VulkanResFactory->RefreshMaterialFrameResources(
					materialHandle,
					m_Context->GetCurrentFrameIndex());

				VulkanGraphicsPipeline* pipeline = GetPipeline(gbufferRt, geometry, material);
				if (!pipeline)
				{
					++skippedInvalidPipelineCount;
					KITA_CORE_WARN(
						"EditorRenderer: object '{}' skipped, failed to get GBuffer pipeline. material handle={}, geometryIndex={}",
						object.GetName(),
						materialHandle,
						static_cast<uint32_t>(i));
					continue;
				}

				BasePassDrawItem item{};
				item.Pipeline = pipeline;
				item.Geometry = geometry.get();
				item.Material = material.get();
				item.PerObject = objectData;

				m_BasePass->AddDrawItem(item);
				++drawItemCount;

				if (m_ViewportPickingPass)
				{
					VulkanGraphicsPipeline* pickingPipeline = GetPickingPipeline(pickingRt, geometry);
					if (!pickingPipeline)
					{
						++skippedInvalidPickingPipelineCount;
						continue;
					}

					ViewportPickingDrawItem pickingItem{};
					pickingItem.Pipeline = pickingPipeline;
					pickingItem.Geometry = geometry.get();
					pickingItem.PushConstants.PerObject = objectData;
					pickingItem.PushConstants.PickID = m_PickRegistry->RegisterSceneObject(object);
					m_ViewportPickingPass->AddDrawItem(pickingItem);
				}
			}
		}
		RenderPassContext gbufferPassContext(*m_Context, cmd, gbufferRt);
		RenderPassContext finalPassContext(*m_Context, cmd, finalRt);
		RenderPassContext pickingPassContext(*m_Context, cmd, pickingRt);
		const uint32_t frameIndex = gbufferPassContext.GetFrameIndex();

		m_BasePass->Execute(gbufferPassContext);
		if (m_DeferredLightingPass)
		{
			m_DeferredLightingPass->UpdateFrameResources(frameIndex);
			m_DeferredLightingPass->SetPipeline(GetDeferredLightingPipeline(finalRt));
			m_DeferredLightingPass->Execute(finalPassContext);
		}
		if (m_SkyboxPass && m_SkyboxMaterial)
		{
			m_SkyboxMaterial->EnsureDescriptors(*m_Context, m_Context->GetFramesInFlight());
			m_SkyboxMaterial->UpdateDescriptorSet(frameIndex);
			m_SkyboxPass->SetMaterial(m_SkyboxMaterial);
			m_SkyboxPass->SetPipeline(GetSkyboxPipeline(finalRt));
			if (m_SkyboxPass->HasValidMaterial())
				m_SkyboxPass->Execute(finalPassContext);
		}
		if (m_EditorGridPass)
		{
			m_EditorGridPass->SetPipeline(GetGridPipeline(finalRt));
			m_EditorGridPass->SetPushConstants(m_GridPushConstants);
			if (m_IsGridEnabled)
				m_EditorGridPass->Execute(finalPassContext);
		}
		if (m_ViewportPickingPass)
		{
			m_ViewportPickingPass->Execute(pickingPassContext);
		}
	}

}
