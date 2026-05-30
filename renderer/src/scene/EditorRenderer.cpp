#include "renderer_pch.h"
#include "EditorRenderer.h"

#include "asset/AssetManager.h"
#include "component/MeshRenderer.h"
#include "component/Transform.h"
#include "project/EditorProjectBootstrap.h"
#include "ui/viewport/EditorPickRegistry.h"
#include "ui/viewport/EditorViewportSurface.h"

#include <backends/imgui_impl_vulkan.h>

namespace Kita {

	EditorRenderer::EditorRenderer(
		VulkanContext& context,
		const RenderGraphTransientRenderTargetDesc& gbufferTargetDesc,
		const RenderGraphTransientRenderTargetDesc& lightingTargetDesc,
		VulkanRenderTarget& finalRt,
		VulkanRenderTarget& pickingRt,
		VulkanResourceFactory& vulkanResFactory,
		PipelineFactory& pipelineFactory,
		const Ref<Scene>& scene,
		ViewportCamera& camera,
		EditorPickRegistry& pickRegistry)
		: m_Context(&context)
		, m_VulkanResFactory(&vulkanResFactory)
		, m_FinalRenderTarget(&finalRt)
		, m_GBufferTargetDesc(gbufferTargetDesc)
		, m_LightingTargetDesc(lightingTargetDesc)
		, m_SceneContext(scene)
		, m_PickRegistry(&pickRegistry)
		, m_ViewportCamera(&camera)
		, m_PipelineFactory(&pipelineFactory)
	{
		Init();
		m_SceneBindings.Init(context, context.GetFramesInFlight());

		m_BasePass = CreateUnique<BasePass>(
			m_SceneBindings, 
			MakeRenderPassDesc(m_GBufferTargetDesc,"BasePass",PassType::GBuffer)
		);

		m_DeferredLightingPass = CreateUnique<DeferredLightingPass>(
			m_SceneBindings, 
			MakeRenderPassDesc(m_LightingTargetDesc,"DeferredLightingPass", PassType::DeferredLighting)
		);

		m_DeferredLightingPass->Init(context, context.GetFramesInFlight());


		m_SkyboxPass = CreateUnique<SkyboxPass>(
			m_SceneBindings,
			MakeRenderPassDesc(m_LightingTargetDesc, "SkyboxPass", PassType::PostProcess));

		m_EditorGridPass = CreateUnique<EditorGridPass>(m_SceneBindings, MakeEditorGridPassDesc(finalRt.CreateView()));
		m_ViewportPickingPass = CreateUnique<ViewportPickingPass>(m_SceneBindings, MakeViewportPickingPassDesc(pickingRt.CreateView()));


		m_TonemapPass = CreateUnique<ToneMappingPass>(m_SceneBindings, MakeTonemappingPassDesc(finalRt.CreateView()));
		m_TonemapPass->Init(context, context.GetFramesInFlight());

		m_RenderGraph = CreateUnique<RenderGraph>();
		InitGridResources();
		InitDeferredLightingResources();
		InitTonemapResources();
	}

	void EditorRenderer::Init()
	{
		const AssetHandle skyboxMaterialHandle = EditorProjectBootstrap::GetPreLoadMaterialHandle("skybox");
		if (Asset::IsValidHandle(skyboxMaterialHandle))
		{
			m_SkyboxMaterial = m_VulkanResFactory->CreateMaterial(skyboxMaterialHandle);
			if (m_SkyboxMaterial)
				m_DefaultSkyboxTexture = m_SkyboxMaterial->GetAlbedoTexture();
		}
	}

	void EditorRenderer::SyncSkyboxMaterialFromSettings()
	{
		if (!m_VulkanResFactory || !m_SceneContext || !m_SkyboxMaterial)
			return;

		const SceneRenderSettings& settings = m_SceneContext->GetRenderSettings();
		Ref<VulkanTexture> skyboxTexture = nullptr;
		if (Asset::IsValidHandle(settings.SkyboxTextureHandle))
			skyboxTexture = m_VulkanResFactory->GetOrCreateTexture(settings.SkyboxTextureHandle);

		if ((!skyboxTexture || !skyboxTexture->IsValid() || skyboxTexture->GetType() != TextureType::TextureCube) &&
			m_IBL && m_IBL->EnvironmentCube && m_IBL->EnvironmentCube->IsValid())
		{
			skyboxTexture = m_IBL->EnvironmentCube;
		}

		if ((!skyboxTexture || !skyboxTexture->IsValid() || skyboxTexture->GetType() != TextureType::TextureCube) &&
			m_DefaultSkyboxTexture && m_DefaultSkyboxTexture->IsValid() && m_DefaultSkyboxTexture->GetType() == TextureType::TextureCube)
		{
			skyboxTexture = m_DefaultSkyboxTexture;
		}

		const Ref<VulkanTexture>& currentTexture = m_SkyboxMaterial->GetAlbedoTexture();
		if (skyboxTexture && skyboxTexture->IsValid() && skyboxTexture->GetType() == TextureType::TextureCube)
		{
			if (currentTexture != skyboxTexture)
				m_SkyboxMaterial->SetAlbedoTexture(skyboxTexture);
		}
	}

	void EditorRenderer::BuildRenderGraph(EditorViewportSurface& surface,VulkanRenderTarget& finalRt, VulkanRenderTarget& pickingRt)
	{
		m_RenderGraph->Reset();
		ResetRenderGraphResourceIDs();


		RenderGraphResourceID finalID = m_RenderGraph->ImportRenderTarget("Final", finalRt);
		RenderGraphResourceID pickingID = m_RenderGraph->ImportRenderTarget("Picking", pickingRt);


		RenderGraphTransientRenderTargetDesc gbufferDesc = m_GBufferTargetDesc;
		gbufferDesc.Width = surface.GetWidth();
		gbufferDesc.Height = surface.GetHeight();

		RenderGraphTransientRenderTarget gbuffer = CreateTransientRenderTarget(*m_RenderGraph, gbufferDesc);
		m_GBufferColorGraphResourceIDs = gbuffer.ColorResourceIDs;
		m_GBufferDepthGraphResourceID = gbuffer.DepthResourceID;


		// Lighting 从这里开始由 RenderGraph 创建 transient image。
		// 当前 RenderGraphTextureDesc 表示单张 image，因此 Lighting color/depth 拆成两个资源。
		RenderGraphTransientRenderTargetDesc lightingDesc = m_LightingTargetDesc;
		lightingDesc.Width = surface.GetWidth();
		lightingDesc.Height = surface.GetHeight();

		RenderGraphTransientRenderTarget lighting = CreateTransientRenderTarget(*m_RenderGraph, lightingDesc);

		KITA_CORE_ASSERT(
			!lighting.ColorAttachments.empty(),
			"Lighting target requires at least one color attachment");
		KITA_CORE_ASSERT(
			lighting.HasDepth(),
			"Lighting target requires depth attachment");


		m_LightingGraphResourceID = lighting.ColorResourceIDs[0];


		m_FinalGraphResourceID = finalID;
		m_PickingGraphResourceID = pickingID;


		const RenderGraphAttachmentRef lightingColor = lighting.ColorAttachments[0];
		const RenderGraphAttachmentRef lightingDepth = lighting.DepthAttachment;

		const RenderGraphAttachmentRef finalColor = RenderGraphAttachmentRef::MakeColor(finalID, 0);
		const RenderGraphAttachmentRef finalDepth = RenderGraphAttachmentRef::MakeDepth(finalID);
		const RenderGraphAttachmentRef pickingColor = RenderGraphAttachmentRef::MakeColor(pickingID, 0);

		RenderGraphPass& gbufferPass = m_RenderGraph->AddPass("GBuffer");
		for (const RenderGraphAttachmentRef& color : gbuffer.ColorAttachments)
		{
			gbufferPass.WriteColor(color);
		}

		gbufferPass.SetExecute([this, gbuffer](RenderGraphContext& graphContext)
			{
				VulkanRenderTargetView gbufferRt = graphContext.BuildRenderTargetView(
					"GBuffer",
					gbuffer.ColorAttachments,
					gbuffer.HasDepth() ? &gbuffer.DepthAttachment : nullptr);

				RenderPassContext passContext(
					graphContext.GetVulkanContext(),
					graphContext.GetCommandBuffer(),
					gbufferRt);

				m_BasePass->Execute(passContext);
			});


		if (m_DeferredLightingPass)
		{
			KITA_CORE_ASSERT(gbuffer.ColorAttachments.size() >= 4, "Current DeferredLighting shader requires at least 4 GBuffer color attachments");
			KITA_CORE_ASSERT(gbuffer.HasDepth(), "Current DeferredLighting shader requires GBuffer depth");


			m_RenderGraph->AddPass("Lighting")
				.ReadTexture(gbuffer.ColorAttachments[0])
				.ReadTexture(gbuffer.ColorAttachments[1])
				.ReadTexture(gbuffer.ColorAttachments[2])
				.ReadTexture(gbuffer.ColorAttachments[3])
				.ReadDepth(gbuffer.DepthAttachment)
				.WriteColor(lightingColor)
				.WriteDepth(lightingDepth)
				.SetExecute([this, gbuffer, lightingColor, lightingDepth](RenderGraphContext& graphContext)
					{
						const uint32_t frameIndex = graphContext.GetVulkanContext().GetCurrentFrameIndex();

						VulkanRenderTargetView gbufferRt = graphContext.BuildRenderTargetView(
							"GBuffer",
							gbuffer.ColorAttachments,
							gbuffer.HasDepth() ? &gbuffer.DepthAttachment : nullptr);

						VulkanRenderTargetView lightingRt = graphContext.BuildRenderTargetView(
							"Lighting",
							{ lightingColor },
							&lightingDepth);


						m_DeferredLightingPass->SetGBufferInput(gbufferRt);
						m_DeferredLightingPass->UpdateFrameResources(frameIndex);
						m_DeferredLightingPass->SetPipeline(GetDeferredLightingPipeline(lightingRt));

						RenderPassContext passContext(
							graphContext.GetVulkanContext(),
							graphContext.GetCommandBuffer(),
							lightingRt);

						m_DeferredLightingPass->Execute(passContext);
					});
		}

		if (m_SkyboxPass && m_SkyboxMaterial)
		{
			m_RenderGraph->AddPass("Skybox")
				.ReadTexture(lightingColor)
				.ReadDepth(lightingDepth)
				.WriteColor(lightingColor)
				.WriteDepth(lightingDepth)
				.SetExecute([this, lightingColor, lightingDepth](RenderGraphContext& graphContext)
					{
						const uint32_t frameIndex = graphContext.GetVulkanContext().GetCurrentFrameIndex();

						VulkanRenderTargetView lightingRt = graphContext.BuildRenderTargetView(
							"Lighting",
							{ lightingColor },
							&lightingDepth);


						SyncSkyboxMaterialFromSettings();
						m_SkyboxMaterial->EnsureDescriptors(
							graphContext.GetVulkanContext(),
							graphContext.GetVulkanContext().GetFramesInFlight());

						if (m_SkyboxMaterial->IsDescriptorSetDirty(frameIndex))
							m_SkyboxMaterial->UpdateDescriptorSet(frameIndex);

						m_SkyboxPass->SetMaterial(m_SkyboxMaterial);
						m_SkyboxPass->SetPipeline(GetSkyboxPipeline(lightingRt));

						if (!m_SkyboxPass->HasValidMaterial())
							return;

						RenderPassContext passContext(
							graphContext.GetVulkanContext(),
							graphContext.GetCommandBuffer(),
							lightingRt);

						m_SkyboxPass->Execute(passContext);
					});
		}

		m_RenderGraph->AddPass("CopyDepth")
			.TransferRead(lightingDepth)
			.TransferWrite(finalDepth)
			.SetExecute([this, lightingColor, lightingDepth, finalColor, finalDepth](RenderGraphContext& graphContext)
				{
					// CopyDepth 只拷贝 depth，但这里仍然通过 RT view 统一拿到 transient/imported attachment。
					VulkanRenderTargetView lightingRt = graphContext.BuildRenderTargetView(
						"Lighting.CopyDepthSource",
						{ lightingColor },
						&lightingDepth);

					VulkanRenderTargetView finalRt = graphContext.BuildRenderTargetView(
						"Final.CopyDepthTarget",
						{ finalColor },
						&finalDepth);

					CopyDepthAttachment(
						lightingRt,
						finalRt,
						graphContext.GetCommandBuffer());
				});

		if (m_TonemapPass)
		{
			m_RenderGraph->AddPass("ToneMap")
				.ReadTexture(lightingColor)
				.ReadDepth(finalDepth)
				.WriteColor(finalColor)
				.WriteDepth(finalDepth)
				.SetExecute([this, lightingColor, lightingDepth, finalColor, finalDepth](RenderGraphContext& graphContext)
					{
						const uint32_t frameIndex = graphContext.GetVulkanContext().GetCurrentFrameIndex();

						VulkanRenderTargetView lightingRt = graphContext.BuildRenderTargetView(
							"Lighting",
							{ lightingColor },
							&lightingDepth);

						VulkanRenderTargetView finalRt = graphContext.BuildRenderTargetView(
							"Final",
							{ finalColor },
							&finalDepth);

						m_TonemapPass->SetSourceInput(lightingRt);
						m_TonemapPass->UpdateFrameResources(frameIndex);
						m_TonemapPass->SetPipeline(GetTonemapPipeline(finalRt));

						RenderPassContext passContext(
							graphContext.GetVulkanContext(),
							graphContext.GetCommandBuffer(),
							finalRt);

						m_TonemapPass->Execute(passContext);
					});
		}

		if (m_EditorGridPass && m_IsGridEnabled)
		{
			m_RenderGraph->AddPass("EditorGrid")
				.ReadTexture(finalColor)
				.ReadDepth(finalDepth)
				.WriteColor(finalColor)
				.WriteDepth(finalDepth)
				.SetExecute([this, finalID](RenderGraphContext& graphContext)
					{
						VulkanRenderTargetView finalRt = graphContext.GetRenderTargetView(finalID);

						m_EditorGridPass->SetPipeline(GetGridPipeline(finalRt));
						m_EditorGridPass->SetPushConstants(m_GridPushConstants);

						RenderPassContext passContext(
							graphContext.GetVulkanContext(),
							graphContext.GetCommandBuffer(),
							finalRt);

						m_EditorGridPass->Execute(passContext);
					});
		}

		if (m_ViewportPickingPass)
		{
			m_RenderGraph->AddPass("Picking")
				.WriteColor(pickingColor)
				.SetExecute([this, pickingID](RenderGraphContext& graphContext)
					{
						RenderPassContext passContext(
							graphContext.GetVulkanContext(),
							graphContext.GetCommandBuffer(),
							graphContext.GetRenderTargetView(pickingID));

						m_ViewportPickingPass->Execute(passContext);
					});
		}
	}

	void EditorRenderer::OnDestroy()
	{
		ReleaseRenderGraphPreviewTextures();

		m_GridVertexShader.reset();
		m_GridFragmentShader.reset();
		m_DeferredLightingVertexShader.reset();
		m_DeferredLightingFragmentShader.reset();
		m_TonemapVertexShader.reset();
		m_TonemapFragmentShader.reset();

		if (m_DeferredLightingPass)
			m_DeferredLightingPass->Destroy();
		if (m_TonemapPass)
			m_TonemapPass->Destroy();
	}

	void EditorRenderer::ResetRenderGraphResourceIDs()
	{
		m_GBufferGraphResourceID = InvalidRenderGraphResourceID;
		m_LightingGraphResourceID = InvalidRenderGraphResourceID;
		m_FinalGraphResourceID = InvalidRenderGraphResourceID;
		m_PickingGraphResourceID = InvalidRenderGraphResourceID;
	}

	void EditorRenderer::UpdateRenderGraphPreviewTextures(const VulkanRenderTarget& finalRt)
	{
		for (RenderGraphPreviewTexture& previewTexture : m_RenderGraphPreviewTextures)
			previewTexture.ActiveThisFrame = false;

		for (uint32_t i = 0; i < static_cast<uint32_t>(m_GBufferColorGraphResourceIDs.size()); ++i)
		{
			const RenderGraphResourceID resourceID = m_GBufferColorGraphResourceIDs[i];
			if (!m_RenderGraph || resourceID == InvalidRenderGraphResourceID)
				continue;

			const VulkanImage* image = m_RenderGraph->GetTransientImage(resourceID);
			if (!image)
				continue;

			RegisterRenderGraphPreviewTexture(
				"GBuffer.Color" + std::to_string(i),
				resourceID,
				i,
				*image);
		}

		// Lighting 已经迁移为 transient，预览直接读取 RenderGraph allocator 当前帧 image。
		if (m_RenderGraph && m_LightingGraphResourceID != InvalidRenderGraphResourceID)
		{
			const VulkanImage* lightingImage = m_RenderGraph->GetTransientImage(m_LightingGraphResourceID);
			if (lightingImage)
			{
				RegisterRenderGraphPreviewTexture(
					"Lighting.Color0",
					m_LightingGraphResourceID,
					0,
					*lightingImage);
			}
		}

		if (finalRt.GetColorAttachmentCount() > 0)
		{
			RegisterRenderGraphPreviewTexture(
				"Final.Color0",
				m_FinalGraphResourceID,
				0,
				finalRt.GetSampledColorAttachment(0));
		}

		RemoveInactiveRenderGraphPreviewTextures();
	}

	void EditorRenderer::RegisterRenderGraphPreviewTexture(
		const std::string& name,
		RenderGraphResourceID resourceID,
		uint32_t attachmentIndex,
		const VulkanImage& image)
	{
		if (!image.IsValid() || !image.HasSampler())
			return;

		KITA_CORE_ASSERT(
			image.GetView() != VK_NULL_HANDLE,
			"RenderGraph preview image view is null");

		RenderGraphPreviewTexture* previewTexture = nullptr;
		for (RenderGraphPreviewTexture& candidate : m_RenderGraphPreviewTextures)
		{
			if (candidate.Name == name)
			{
				previewTexture = &candidate;
				break;
			}
		}

		if (!previewTexture)
		{
			m_RenderGraphPreviewTextures.push_back({});
			previewTexture = &m_RenderGraphPreviewTextures.back();
			previewTexture->Name = name;
		}

		const VkExtent3D extent = image.GetExtent();
		const bool descriptorChanged =
			previewTexture->ImageHandle != image.GetHandle() ||
			!previewTexture->TextureID;

		if (descriptorChanged)
		{
			ReleaseRenderGraphPreviewTexture(*previewTexture);
			previewTexture->TextureID = static_cast<ImTextureID>(reinterpret_cast<uint64_t>(
				ImGui_ImplVulkan_AddTexture(
					image.GetSampler(),
					image.GetView(),
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)));
		}

		previewTexture->ResourceID = resourceID;
		previewTexture->AttachmentIndex = attachmentIndex;
		previewTexture->ImageHandle = image.GetHandle();
		previewTexture->Width = extent.width;
		previewTexture->Height = extent.height;
		previewTexture->Format = image.GetFormat();
		previewTexture->ActiveThisFrame = true;
	}

	void EditorRenderer::ReleaseRenderGraphPreviewTexture(RenderGraphPreviewTexture& previewTexture)
	{
		if (!previewTexture.TextureID)
			return;

		ImGui_ImplVulkan_RemoveTexture(
			reinterpret_cast<VkDescriptorSet>(
				static_cast<uint64_t>(previewTexture.TextureID)));
		previewTexture.TextureID = 0;
		previewTexture.ImageHandle = VK_NULL_HANDLE;
	}

	void EditorRenderer::ReleaseRenderGraphPreviewTextures()
	{
		for (RenderGraphPreviewTexture& previewTexture : m_RenderGraphPreviewTextures)
			ReleaseRenderGraphPreviewTexture(previewTexture);

		m_RenderGraphPreviewTextures.clear();
	}

	void EditorRenderer::RemoveInactiveRenderGraphPreviewTextures()
	{
		for (auto it = m_RenderGraphPreviewTextures.begin(); it != m_RenderGraphPreviewTextures.end();)
		{
			if (it->ActiveThisFrame)
			{
				++it;
				continue;
			}

			ReleaseRenderGraphPreviewTexture(*it);
			it = m_RenderGraphPreviewTextures.erase(it);
		}
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

	void EditorRenderer::InitTonemapResources()
	{
		if (!m_Context || !m_TonemapPass)
			return;

		const AssetHandle tonemapShaderHandle = EditorProjectBootstrap::GetPreLoadShaderHandle("tonemap");
		if (!Asset::IsValidHandle(tonemapShaderHandle))
		{
			KITA_CORE_WARN("EditorRenderer: preload shader handle 'tonemap' is invalid.");
			return;
		}

		VulkanResourceFactory::ShaderBundle shaderBundle = m_VulkanResFactory->GetOrCreateShaderBundle(tonemapShaderHandle);
		if (!shaderBundle.IsValid())
		{
			KITA_CORE_WARN("EditorRenderer: failed to create shader bundle for preload shader 'tonemap'.");
			return;
		}

		m_TonemapVertexShader = shaderBundle.VertexShader;
		m_TonemapFragmentShader = shaderBundle.FragmentShader;
	}

VulkanGraphicsPipeline* EditorRenderer::GetPipeline(const RenderGraphTransientRenderTargetDesc& targetDesc, Ref<VulkanGeometry>& geometry, Ref<VulkanMaterial>& material)
	{
		PipelineRequest request{};
		request.Pass = PassType::GBuffer;
		request.Geometry = geometry.get();
		request.VertexShader = material->GetVertexShader().get();
		request.FragmentShader = material->GetFragmentShader().get();

		request.ColorFormats.clear();
		request.ColorFormats.reserve(targetDesc.Colors.size());
		for (const RenderGraphColorAttachmentDesc& color : targetDesc.Colors)
			request.ColorFormats.push_back(color.Format);

		request.DepthFormat = targetDesc.Depth.Enabled ? targetDesc.Depth.Format : VK_FORMAT_UNDEFINED;
		request.Samples = targetDesc.Samples;
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

		return m_PipelineFactory->GetOrCreate(request);
	}

VulkanGraphicsPipeline* EditorRenderer::GetDeferredLightingPipeline(const VulkanRenderTargetView& rt)
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
		request.Samples = rt.GetSamples();
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

VulkanGraphicsPipeline* EditorRenderer::GetGridPipeline(const VulkanRenderTargetView& rt)
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
		request.Samples = rt.GetSamples();
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

VulkanGraphicsPipeline* EditorRenderer::GetTonemapPipeline(const VulkanRenderTargetView& rt)
	{
		if (!m_TonemapVertexShader || !m_TonemapFragmentShader || !m_TonemapPass)
			return nullptr;

		PipelineRequest request{};
		request.Pass = PassType::PostProcess;
		request.UseVertexInput = false;
		request.VertexShader = m_TonemapVertexShader.get();
		request.FragmentShader = m_TonemapFragmentShader.get();

		request.ColorFormats.clear();
		for (uint32_t i = 0; i < rt.GetColorAttachmentCount(); ++i)
			request.ColorFormats.push_back(rt.GetColorFormat(i));

		request.DepthFormat = rt.HasDepthAttachment() ? rt.GetDepthFormat() : VK_FORMAT_UNDEFINED;
		request.Samples = rt.GetSamples();
		request.DescriptorSetLayouts = {
			m_SceneBindings.GetDescriptorSet(0).GetLayout(),
			m_TonemapPass->GetDescriptorSet(0).GetLayout()
		};
		request.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		request.PolygonMode = VK_POLYGON_MODE_FILL;
		request.CullMode = VK_CULL_MODE_NONE;
		request.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		request.EnableDepthTest = false;
		request.EnableDepthWrite = false;
		request.DepthCompareOp = VK_COMPARE_OP_ALWAYS;
		request.EnableBlending = false;
		request.PushConstantStages = 0;
		request.PushConstantSize = 0;

		return m_PipelineFactory->GetOrCreate(request);
	}

VulkanGraphicsPipeline* EditorRenderer::GetPickingPipeline(const VulkanRenderTargetView& rt, Ref<VulkanGeometry>& geometry)
	{
		if (!m_ViewportPickingPass)
			return nullptr;

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
		request.Samples = rt.GetSamples();
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

VulkanGraphicsPipeline* EditorRenderer::GetSkyboxPipeline(const VulkanRenderTargetView& rt)
	{
		if (!m_SkyboxMaterial || !m_SkyboxMaterial->GetVertexShader() || !m_SkyboxMaterial->GetFragmentShader())
			return nullptr;

		PipelineRequest request{};
		request.Pass = PassType::PostProcess;
		request.UseVertexInput = false;
		request.VertexShader = m_SkyboxMaterial->GetVertexShader().get();
		request.FragmentShader = m_SkyboxMaterial->GetFragmentShader().get();

		request.ColorFormats.clear();
		for (uint32_t i = 0; i < rt.GetColorAttachmentCount(); ++i)
			request.ColorFormats.push_back(rt.GetColorFormat(i));
		request.DepthFormat = rt.HasDepthAttachment() ? rt.GetDepthFormat() : VK_FORMAT_UNDEFINED;
		request.Samples = rt.GetSamples();
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
		request.PushConstantStages = VK_SHADER_STAGE_FRAGMENT_BIT;
		request.PushConstantSize = SkyboxPushConstantSize;

		return m_PipelineFactory->GetOrCreate(request);
	}

	void EditorRenderer::CopyDepthAttachment(
		const VulkanRenderTargetView& sourceRt,
		const VulkanRenderTargetView& targetRt,
		VkCommandBuffer commandBuffer) const
	{
		if (!sourceRt.HasDepthAttachment() || !targetRt.HasDepthAttachment())
			return;

		const VulkanImage* sourceDepth = sourceRt.GetDepthAttachment();
		const VulkanImage* targetDepth = targetRt.GetDepthAttachment();
		if (!sourceDepth || !targetDepth)
			return;

		VulkanImage& mutableSourceDepth = const_cast<VulkanImage&>(*sourceDepth);
		VulkanImage& mutableTargetDepth = const_cast<VulkanImage&>(*targetDepth);

		const VkImageLayout sourcePreviousLayout = mutableSourceDepth.GetCurrentLayout();
		const VkImageLayout targetPreviousLayout = mutableTargetDepth.GetCurrentLayout();

		mutableSourceDepth.TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		mutableTargetDepth.TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkImageCopy copyRegion{};
		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		copyRegion.srcSubresource.mipLevel = 0;
		copyRegion.srcSubresource.baseArrayLayer = 0;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		copyRegion.dstSubresource.mipLevel = 0;
		copyRegion.dstSubresource.baseArrayLayer = 0;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.extent = {
			std::min(sourceRt.GetWidth(), targetRt.GetWidth()),
			std::min(sourceRt.GetHeight(), targetRt.GetHeight()),
			1
		};

		vkCmdCopyImage(
			commandBuffer,
			mutableSourceDepth.GetHandle(),
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			mutableTargetDepth.GetHandle(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&copyRegion);

		if (sourcePreviousLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
			mutableSourceDepth.TransitionLayout(commandBuffer, sourcePreviousLayout);

		const VkImageLayout targetFinalLayout =
			targetPreviousLayout == VK_IMAGE_LAYOUT_UNDEFINED
			? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
			: targetPreviousLayout;
		if (targetFinalLayout != VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
			mutableTargetDepth.TransitionLayout(commandBuffer, targetFinalLayout);
	}

	void EditorRenderer::Render(EditorViewportSurface& surface)
	{
		if (!m_Context || !m_SceneContext || !m_ViewportCamera || !m_PickRegistry || !m_RenderGraph)
			return;

		auto cmd = m_Context->GetCurrentCommandBuffer();
		if (cmd == VK_NULL_HANDLE)
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
			lightingSceneData.BeginInfo.TransitionSampledDepth = true;
			m_DeferredLightingPass->SetSceneData(lightingSceneData);
			m_DeferredLightingPass->SetGBufferInput(gbufferRt.CreateView());
			m_DeferredLightingPass->SetIBLInput(m_IBL);
		}

		if (m_TonemapPass)
		{
			ScenePassData tonemapSceneData = sceneData;
			tonemapSceneData.BeginInfo.ClearColors = true;
			tonemapSceneData.BeginInfo.ClearDepthAttachment = false;
			tonemapSceneData.BeginInfo.TransitionSampledColors = true;
			tonemapSceneData.BeginInfo.TransitionSampledDepth = false;
			m_TonemapPass->SetSceneData(tonemapSceneData);
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
			const SceneRenderSettings& skyboxSettings = m_SceneContext->GetRenderSettings();
			SkyboxPushConstants pushConstants{};
			pushConstants.Intensity = skyboxSettings.SkyboxIntensity;
			pushConstants.RotationY = skyboxSettings.SkyboxRotationY;
			pushConstants.MipLevel = skyboxSettings.SkyboxMipLevel;
			m_SkyboxPass->SetPushConstants(pushConstants);
		}

		if (m_ViewportPickingPass)
		{
			ScenePassData pickingSceneData = sceneData;
			pickingSceneData.BeginInfo.ClearColor = glm::vec4(0.0f);
			pickingSceneData.BeginInfo.TransitionSampledColors = false;
			pickingSceneData.BeginInfo.TransitionSampledDepth = false;
			m_ViewportPickingPass->SetSceneData(pickingSceneData);
		}




		m_BasePass->ClearDrawItems();
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
				if (!material || !material->GetVertexShader() || !material->GetFragmentShader())
					continue;

				m_VulkanResFactory->RefreshMaterialFrameResources(
					materialHandle,
					m_Context->GetCurrentFrameIndex());

				RenderGraphTransientRenderTargetDesc gbufferDesc = m_GBufferTargetDesc;
				gbufferDesc.Width = surface.GetWidth();
				gbufferDesc.Height = surface.GetHeight();

				VulkanGraphicsPipeline* pipeline = GetPipeline(gbufferDesc, geometry, material);

				if (!pipeline)
					continue;

				BasePassDrawItem item{};
				item.Pipeline = pipeline;
				item.Geometry = geometry.get();
				item.Material = material.get();
				item.PerObject = objectData;
				m_BasePass->AddDrawItem(item);

				if (m_ViewportPickingPass)
				{
					VulkanGraphicsPipeline* pickingPipeline = GetPickingPipeline(pickingRt.CreateView(), geometry);
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


		BuildRenderGraph(surface,finalRt, pickingRt);
		m_RenderGraph->Execute(*m_Context, cmd);
		UpdateRenderGraphPreviewTextures(finalRt);
	}

}
