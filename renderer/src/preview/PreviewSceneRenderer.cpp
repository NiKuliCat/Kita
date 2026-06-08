#include "renderer_pch.h"
#include "PreviewSceneRenderer.h"

#include "asset/AssetManager.h"
#include "file/Project.h"
#include "project/EditorProjectBootstrap.h"
#include "render/deferred/DeferredLightingUberShaderBuilder.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace Kita {

	namespace
	{
		constexpr const char* kCubemapPreviewShaderLabPath = "packages/render/shaders/CubemapPreview.shader";
		constexpr const char* kSkyboxShaderLabPath = "packages/render/shaders/Skybox.shader";

		RenderPassBeginInfo MakeBeginInfo(
			bool clearColors,
			bool clearDepth,
			const glm::vec4& clearColor)
		{
			RenderPassBeginInfo beginInfo{};
			beginInfo.ClearColor = clearColor;
			beginInfo.ClearDepth = 1.0f;
			beginInfo.ClearStencil = 0;
			beginInfo.ClearColors = clearColors;
			beginInfo.ClearDepthAttachment = clearDepth;
			beginInfo.TransitionSampledColors = true;
			beginInfo.TransitionSampledDepth = false;
			return beginInfo;
		}

		glm::vec3 BuildCameraPosition(const PreviewOrbitCameraState& state)
		{
			const float yaw = glm::radians(state.YawDegrees);
			const float pitch = glm::radians(state.PitchDegrees);
			const float distance = std::max(0.6f, state.Distance);

			const glm::vec3 forward(
				std::cos(pitch) * std::sin(yaw),
				std::sin(pitch),
				std::cos(pitch) * std::cos(yaw));
			return state.Target - glm::normalize(forward) * distance;
		}
	}

	PreviewSceneRenderer::PreviewSceneRenderer(
		VulkanContext& context,
		VulkanResourceFactory& resourceFactory,
		PipelineFactory& pipelineFactory)
		: m_Context(&context)
		, m_ResourceFactory(&resourceFactory)
		, m_PipelineFactory(&pipelineFactory)
	{
	}

	PreviewSceneRenderer::~PreviewSceneRenderer()
	{
		Clear();
	}

	void PreviewSceneRenderer::Invalidate(AssetHandle handle)
	{
		m_CubemapPreviewMaterials.erase(handle);
	}

	void PreviewSceneRenderer::Clear()
	{
		m_CubemapPreviewMaterials.clear();
		m_PreviewSkyboxMaterial = nullptr;
		m_DefaultSkyboxTexture = nullptr;
		m_SphereGeometry = nullptr;
		m_BasePass.reset();
		m_DeferredLightingPass.reset();
		m_ForwardOpaquePass.reset();
		m_TonemapPass.reset();
		m_SkyboxPass.reset();
		m_SceneBindings.reset();
		m_OffscreenTargets = {};
		m_DeferredLightingVertexShader = nullptr;
		m_DeferredLightingFragmentShader = nullptr;
		m_TonemapVertexShader = nullptr;
		m_TonemapFragmentShader = nullptr;
	}

	bool PreviewSceneRenderer::RenderMaterialDefinitionPreview(
		AssetHandle materialDefinitionHandle,
		EditorViewportSurface& surface,
		const PreviewOrbitCameraState& cameraState)
	{
		if (!Asset::IsValidHandle(materialDefinitionHandle))
		{
			return false;
		}

		MaterialAsset transientMaterial{};
		transientMaterial.MaterialDefinitionHandle = materialDefinitionHandle;
		Ref<VulkanMaterial> runtimeMaterial = m_ResourceFactory ? m_ResourceFactory->CreateMaterial(transientMaterial) : nullptr;
		if (!runtimeMaterial)
		{
			return false;
		}

		// 复用实例预览主路径，只是输入改成临时母材质实例。
		const uint32_t width = std::max(1u, surface.GetWidth());
		const uint32_t height = std::max(1u, surface.GetHeight());

		if (!m_Context || !m_ResourceFactory || !m_PipelineFactory || !EnsureSharedResources() || !EnsureOffscreenTargets(width, height))
		{
			return false;
		}

		const uint32_t frameIndex = m_Context->GetCurrentFrameIndex();
		runtimeMaterial->EnsureDescriptors(*m_Context, m_Context->GetFramesInFlight());
		if (runtimeMaterial->IsDescriptorSetDirty(frameIndex))
		{
			runtimeMaterial->UpdateDescriptorSet(frameIndex);
		}

		const bool hasGBufferPass = HasMaterialPass(*runtimeMaterial, PassType::GBuffer);
		const bool hasForwardPass = HasMaterialPass(*runtimeMaterial, PassType::ForwardOpaque);
		if (!hasGBufferPass && !hasForwardPass)
		{
			return false;
		}

		VulkanRenderTargetView gbufferView = m_OffscreenTargets.GBuffer->CreateView();
		VulkanRenderTargetView lightingView = m_OffscreenTargets.Lighting->CreateView();
		VulkanRenderTargetView finalView = surface.GetFinalRenderTarget().CreateView();

		if (!m_BasePass)
		{
			m_BasePass = CreateUnique<BasePass>(*m_SceneBindings, MakeBasePassDesc(gbufferView));
		}
		if (!m_DeferredLightingPass)
		{
			m_DeferredLightingPass = CreateUnique<DeferredLightingPass>(*m_SceneBindings, MakeDeferredLightingPassDesc(lightingView));
			m_DeferredLightingPass->Init(*m_Context, m_Context->GetFramesInFlight());
		}
		if (!m_ForwardOpaquePass)
		{
			m_ForwardOpaquePass = CreateUnique<ForwardOpaquePass>(*m_SceneBindings, MakeForwardOpaquePassDesc(lightingView));
		}
		if (!m_TonemapPass)
		{
			m_TonemapPass = CreateUnique<ToneMappingPass>(*m_SceneBindings, MakeTonemappingPassDesc(finalView));
			m_TonemapPass->Init(*m_Context, m_Context->GetFramesInFlight());
		}
		if (!m_SkyboxPass)
		{
			m_SkyboxPass = CreateUnique<SkyboxPass>(*m_SceneBindings, MakeSkyboxPassDesc(lightingView));
		}

		m_BasePass->ClearDrawItems();
		m_ForwardOpaquePass->ClearDrawItems();

		const ObjectData previewObject = BuildPreviewObjectData();
		const ScenePassData gbufferSceneData = BuildSceneData(cameraState, width, height, MakeBeginInfo(true, true, glm::vec4(0.0f)));
		const ScenePassData lightingSceneData = BuildSceneData(cameraState, width, height, MakeBeginInfo(true, true, glm::vec4(0.045f, 0.048f, 0.055f, 1.0f)));
		const ScenePassData lightingOverlaySceneData = BuildSceneData(cameraState, width, height, MakeBeginInfo(false, false, glm::vec4(0.0f)));
		const ScenePassData tonemapSceneData = BuildSceneData(cameraState, width, height, MakeBeginInfo(true, false, glm::vec4(0.0f)));

		if (hasGBufferPass)
		{
			VulkanGraphicsPipeline* gbufferPipeline = GetMaterialPipeline(gbufferView, *m_SphereGeometry, *runtimeMaterial, PassType::GBuffer);
			if (!gbufferPipeline)
			{
				return false;
			}

			BasePassDrawItem gbufferItem{};
			gbufferItem.Pipeline = gbufferPipeline;
			gbufferItem.Geometry = m_SphereGeometry.get();
			gbufferItem.Material = runtimeMaterial.get();
			gbufferItem.PerObject = previewObject;
			m_BasePass->AddDrawItem(gbufferItem);
			m_BasePass->SetSceneData(gbufferSceneData);

			RenderPassContext gbufferContext(*m_Context, m_Context->GetCurrentCommandBuffer(), gbufferView);
			m_BasePass->Execute(gbufferContext);

			m_DeferredLightingPass->SetSceneData(lightingSceneData);
			m_DeferredLightingPass->SetGBufferInput(gbufferView);
			m_DeferredLightingPass->SetIBLInput(m_IBL);
			m_DeferredLightingPass->UpdateFrameResources(frameIndex);
			m_DeferredLightingPass->SetPipeline(GetDeferredLightingPipeline(lightingView));

			RenderPassContext lightingContext(*m_Context, m_Context->GetCurrentCommandBuffer(), lightingView);
			m_DeferredLightingPass->Execute(lightingContext);
		}

		Ref<VulkanMaterial> skyboxMaterial = GetOrCreatePreviewSkyboxMaterial();
		if (skyboxMaterial && m_SkyboxPass)
		{
			Ref<VulkanTexture> skyboxTexture = nullptr;
			if (m_IBL && m_IBL->IsValid() && m_IBL->EnvironmentCube && m_IBL->EnvironmentCube->IsValid())
			{
				skyboxTexture = m_IBL->EnvironmentCube;
			}
			else if (m_DefaultSkyboxTexture && m_DefaultSkyboxTexture->IsValid())
			{
				skyboxTexture = m_DefaultSkyboxTexture;
			}

			if (skyboxTexture && skyboxTexture->IsValid() && skyboxTexture->GetType() == TextureType::TextureCube)
			{
				skyboxMaterial->SetAlbedoTexture(skyboxTexture);
				skyboxMaterial->SetResolvedTexture("_Albedo", skyboxTexture);
				skyboxMaterial->EnsureDescriptors(*m_Context, m_Context->GetFramesInFlight());
				if (skyboxMaterial->IsDescriptorSetDirty(frameIndex))
				{
					skyboxMaterial->UpdateDescriptorSet(frameIndex);
				}

				m_SkyboxPass->SetMaterial(skyboxMaterial);
				SkyboxPushConstants skyboxConstants{};
				skyboxConstants.Intensity = 1.0f;
				skyboxConstants.RotationY = 0.0f;
				skyboxConstants.MipLevel = 0.0f;
				m_SkyboxPass->SetPushConstants(skyboxConstants);
				m_SkyboxPass->SetSceneData(hasGBufferPass ? lightingOverlaySceneData : lightingSceneData);
				m_SkyboxPass->SetPipeline(GetSkyboxPipeline(lightingView, *skyboxMaterial));

				if (m_SkyboxPass->HasValidMaterial())
				{
					RenderPassContext skyboxContext(*m_Context, m_Context->GetCurrentCommandBuffer(), lightingView);
					m_SkyboxPass->Execute(skyboxContext);
				}
			}
		}

		if (!hasGBufferPass && hasForwardPass)
		{
			VulkanGraphicsPipeline* forwardPipeline = GetMaterialPipeline(lightingView, *m_SphereGeometry, *runtimeMaterial, PassType::ForwardOpaque);
			if (!forwardPipeline)
			{
				return false;
			}

			ForwardOpaqueDrawItem forwardItem{};
			forwardItem.Pipeline = forwardPipeline;
			forwardItem.Geometry = m_SphereGeometry.get();
			forwardItem.Material = runtimeMaterial.get();
			forwardItem.PerObject = previewObject;
			m_ForwardOpaquePass->AddDrawItem(forwardItem);
			m_ForwardOpaquePass->SetSceneData(lightingOverlaySceneData);

			RenderPassContext forwardContext(*m_Context, m_Context->GetCurrentCommandBuffer(), lightingView);
			m_ForwardOpaquePass->Execute(forwardContext);
		}

		m_TonemapPass->SetSceneData(tonemapSceneData);
		m_TonemapPass->SetSourceInput(lightingView);
		m_TonemapPass->UpdateFrameResources(frameIndex);
		m_TonemapPass->SetPipeline(GetTonemapPipeline(finalView));

		RenderPassContext tonemapContext(*m_Context, m_Context->GetCurrentCommandBuffer(), finalView);
		m_TonemapPass->Execute(tonemapContext);
		return true;
	}

	bool PreviewSceneRenderer::RenderMaterialPreview(
		AssetHandle materialHandle,
		EditorViewportSurface& surface,
		const PreviewOrbitCameraState& cameraState)
	{
		if (!m_Context || !m_ResourceFactory || !m_PipelineFactory || !Asset::IsValidHandle(materialHandle))
		{
			return false;
		}

		if (!EnsureSharedResources())
		{
			return false;
		}

		const uint32_t width = std::max(1u, surface.GetWidth());
		const uint32_t height = std::max(1u, surface.GetHeight());
		if (!EnsureOffscreenTargets(width, height))
		{
			return false;
		}

		Ref<VulkanMaterial> material = m_ResourceFactory->CreateMaterial(materialHandle);
		if (!material)
		{
			return false;
		}

		const uint32_t frameIndex = m_Context->GetCurrentFrameIndex();
		m_ResourceFactory->RefreshMaterialFrameResources(materialHandle, frameIndex);
		if (!material->HasDescriptorSets())
		{
			return false;
		}

		const bool hasGBufferPass = HasMaterialPass(*material, PassType::GBuffer);
		const bool hasForwardPass = HasMaterialPass(*material, PassType::ForwardOpaque);
		if (!hasGBufferPass && !hasForwardPass)
		{
			return false;
		}

		VulkanRenderTargetView gbufferView = m_OffscreenTargets.GBuffer->CreateView();
		VulkanRenderTargetView lightingView = m_OffscreenTargets.Lighting->CreateView();
		VulkanRenderTargetView finalView = surface.GetFinalRenderTarget().CreateView();

		if (!m_BasePass)
		{
			m_BasePass = CreateUnique<BasePass>(*m_SceneBindings, MakeBasePassDesc(gbufferView));
		}
		if (!m_DeferredLightingPass)
		{
			m_DeferredLightingPass = CreateUnique<DeferredLightingPass>(*m_SceneBindings, MakeDeferredLightingPassDesc(lightingView));
			m_DeferredLightingPass->Init(*m_Context, m_Context->GetFramesInFlight());
		}
		if (!m_ForwardOpaquePass)
		{
			m_ForwardOpaquePass = CreateUnique<ForwardOpaquePass>(*m_SceneBindings, MakeForwardOpaquePassDesc(lightingView));
		}
		if (!m_TonemapPass)
		{
			m_TonemapPass = CreateUnique<ToneMappingPass>(*m_SceneBindings, MakeTonemappingPassDesc(finalView));
			m_TonemapPass->Init(*m_Context, m_Context->GetFramesInFlight());
		}
		if (!m_SkyboxPass)
		{
			m_SkyboxPass = CreateUnique<SkyboxPass>(*m_SceneBindings, MakeSkyboxPassDesc(lightingView));
		}

		m_BasePass->ClearDrawItems();
		m_ForwardOpaquePass->ClearDrawItems();

		const ObjectData previewObject = BuildPreviewObjectData();
		const ScenePassData gbufferSceneData = BuildSceneData(
			cameraState,
			width,
			height,
			MakeBeginInfo(true, true, glm::vec4(0.0f)));
		const ScenePassData lightingSceneData = BuildSceneData(
			cameraState,
			width,
			height,
			MakeBeginInfo(true, true, glm::vec4(0.045f, 0.048f, 0.055f, 1.0f)));
		const ScenePassData lightingOverlaySceneData = BuildSceneData(
			cameraState,
			width,
			height,
			MakeBeginInfo(false, false, glm::vec4(0.0f)));
		const ScenePassData tonemapSceneData = BuildSceneData(
			cameraState,
			width,
			height,
			MakeBeginInfo(true, false, glm::vec4(0.0f)));

		if (hasGBufferPass)
		{
			VulkanGraphicsPipeline* gbufferPipeline =
				GetMaterialPipeline(gbufferView, *m_SphereGeometry, *material, PassType::GBuffer);
			if (!gbufferPipeline)
			{
				return false;
			}

			BasePassDrawItem gbufferItem{};
			gbufferItem.Pipeline = gbufferPipeline;
			gbufferItem.Geometry = m_SphereGeometry.get();
			gbufferItem.Material = material.get();
			gbufferItem.PerObject = previewObject;
			m_BasePass->AddDrawItem(gbufferItem);
			m_BasePass->SetSceneData(gbufferSceneData);

			RenderPassContext gbufferContext(*m_Context, m_Context->GetCurrentCommandBuffer(), gbufferView);
			m_BasePass->Execute(gbufferContext);

			m_DeferredLightingPass->SetSceneData(lightingSceneData);
			m_DeferredLightingPass->SetGBufferInput(gbufferView);
			m_DeferredLightingPass->SetIBLInput(m_IBL);
			m_DeferredLightingPass->UpdateFrameResources(frameIndex);
			m_DeferredLightingPass->SetPipeline(GetDeferredLightingPipeline(lightingView));

			RenderPassContext lightingContext(*m_Context, m_Context->GetCurrentCommandBuffer(), lightingView);
			m_DeferredLightingPass->Execute(lightingContext);
		}

		Ref<VulkanMaterial> skyboxMaterial = GetOrCreatePreviewSkyboxMaterial();
		if (skyboxMaterial && m_SkyboxPass)
		{
			Ref<VulkanTexture> skyboxTexture = nullptr;
			if (m_IBL && m_IBL->IsValid() && m_IBL->EnvironmentCube && m_IBL->EnvironmentCube->IsValid())
			{
				skyboxTexture = m_IBL->EnvironmentCube;
			}
			else if (m_DefaultSkyboxTexture && m_DefaultSkyboxTexture->IsValid())
			{
				skyboxTexture = m_DefaultSkyboxTexture;
			}

			if (skyboxTexture && skyboxTexture->IsValid() && skyboxTexture->GetType() == TextureType::TextureCube)
			{
				skyboxMaterial->SetAlbedoTexture(skyboxTexture);
				skyboxMaterial->SetResolvedTexture("_Albedo", skyboxTexture);
				skyboxMaterial->EnsureDescriptors(*m_Context, m_Context->GetFramesInFlight());
				if (skyboxMaterial->IsDescriptorSetDirty(frameIndex))
				{
					skyboxMaterial->UpdateDescriptorSet(frameIndex);
				}

				m_SkyboxPass->SetMaterial(skyboxMaterial);
				SkyboxPushConstants skyboxConstants{};
				skyboxConstants.Intensity = 1.0f;
				skyboxConstants.RotationY = 0.0f;
				skyboxConstants.MipLevel = 0.0f;
				m_SkyboxPass->SetPushConstants(skyboxConstants);
				m_SkyboxPass->SetSceneData(hasGBufferPass ? lightingOverlaySceneData : lightingSceneData);
				m_SkyboxPass->SetPipeline(GetSkyboxPipeline(lightingView, *skyboxMaterial));

				if (m_SkyboxPass->HasValidMaterial())
				{
					RenderPassContext skyboxContext(*m_Context, m_Context->GetCurrentCommandBuffer(), lightingView);
					m_SkyboxPass->Execute(skyboxContext);
				}
			}
		}

		if (!hasGBufferPass && hasForwardPass)
		{
			VulkanGraphicsPipeline* forwardPipeline =
				GetMaterialPipeline(lightingView, *m_SphereGeometry, *material, PassType::ForwardOpaque);
			if (!forwardPipeline)
			{
				return false;
			}

			ForwardOpaqueDrawItem forwardItem{};
			forwardItem.Pipeline = forwardPipeline;
			forwardItem.Geometry = m_SphereGeometry.get();
			forwardItem.Material = material.get();
			forwardItem.PerObject = previewObject;
			m_ForwardOpaquePass->AddDrawItem(forwardItem);
			m_ForwardOpaquePass->SetSceneData(lightingOverlaySceneData);

			RenderPassContext forwardContext(*m_Context, m_Context->GetCurrentCommandBuffer(), lightingView);
			m_ForwardOpaquePass->Execute(forwardContext);
		}

		m_TonemapPass->SetSceneData(tonemapSceneData);
		m_TonemapPass->SetSourceInput(lightingView);
		m_TonemapPass->UpdateFrameResources(frameIndex);
		m_TonemapPass->SetPipeline(GetTonemapPipeline(finalView));

		RenderPassContext tonemapContext(*m_Context, m_Context->GetCurrentCommandBuffer(), finalView);
		m_TonemapPass->Execute(tonemapContext);
		return true;
	}

	bool PreviewSceneRenderer::RenderCubemapPreview(
		AssetHandle textureHandle,
		EditorViewportSurface& surface,
		const PreviewOrbitCameraState& cameraState)
	{
		if (!m_Context || !m_ResourceFactory || !m_PipelineFactory || !Asset::IsValidHandle(textureHandle))
		{
			return false;
		}

		if (!EnsureSharedResources())
		{
			return false;
		}

		Ref<VulkanTexture> texture = m_ResourceFactory->GetOrCreateTexture(textureHandle);
		if (!texture || !texture->IsValid() || texture->GetType() != TextureType::TextureCube)
		{
			return false;
		}

		const uint32_t frameIndex = m_Context->GetCurrentFrameIndex();
		Ref<VulkanMaterial> previewMaterial = BuildTransientCubemapPreviewMaterial(textureHandle, texture);
		if (!previewMaterial)
		{
			return false;
		}

		previewMaterial->EnsureDescriptors(*m_Context, m_Context->GetFramesInFlight());
		if (previewMaterial->IsDescriptorSetDirty(frameIndex))
		{
			previewMaterial->UpdateDescriptorSet(frameIndex);
		}

		VulkanRenderTargetView finalView = surface.GetFinalRenderTarget().CreateView();
		ForwardOpaquePass cubemapPass(*m_SceneBindings, MakeForwardOpaquePassDesc(finalView));
		VulkanGraphicsPipeline* previewPipeline =
			GetMaterialPipeline(finalView, *m_SphereGeometry, *previewMaterial, PassType::ForwardOpaque);
		if (!previewPipeline)
		{
			return false;
		}

		ForwardOpaqueDrawItem drawItem{};
		drawItem.Pipeline = previewPipeline;
		drawItem.Geometry = m_SphereGeometry.get();
		drawItem.Material = previewMaterial.get();
		drawItem.PerObject = BuildPreviewObjectData();
		cubemapPass.AddDrawItem(drawItem);
		cubemapPass.SetSceneData(BuildSceneData(
			cameraState,
			surface.GetWidth(),
			surface.GetHeight(),
			MakeBeginInfo(true, true, glm::vec4(0.055f, 0.058f, 0.065f, 1.0f))));

		RenderPassContext passContext(*m_Context, m_Context->GetCurrentCommandBuffer(), finalView);
		cubemapPass.Execute(passContext);
		return true;
	}

	bool PreviewSceneRenderer::EnsureSharedResources()
	{
		if (!m_Context || !m_ResourceFactory || !m_PipelineFactory)
		{
			return false;
		}

		if (!m_SceneBindings)
		{
			m_SceneBindings = CreateUnique<SceneBindings>();
			m_SceneBindings->Init(*m_Context, m_Context->GetFramesInFlight());
		}

		if (!m_SphereGeometry)
		{
			const AssetHandle sphereMeshHandle = EditorProjectBootstrap::GetPreLoadMeshHandle("sphere");
			if (!Asset::IsValidHandle(sphereMeshHandle))
			{
				return false;
			}

			std::vector<Ref<VulkanGeometry>> geometries = m_ResourceFactory->GetOrCreateGeometries(sphereMeshHandle);
			if (geometries.empty() || !geometries.front())
			{
				return false;
			}

			m_SphereGeometry = geometries.front();
		}

		if (!m_TonemapVertexShader || !m_TonemapFragmentShader)
		{
			const AssetHandle tonemapShaderHandle = EditorProjectBootstrap::GetPreLoadShaderHandle("tonemap");
			if (Asset::IsValidHandle(tonemapShaderHandle))
			{
				VulkanResourceFactory::ShaderBundle tonemapBundle =
					m_ResourceFactory->BuildShaderLabPassBundle(tonemapShaderHandle, PassType::PostProcess);
				if (tonemapBundle.IsValid())
				{
					m_TonemapVertexShader = tonemapBundle.VertexShader;
					m_TonemapFragmentShader = tonemapBundle.FragmentShader;
				}
			}
		}

		if (!m_DeferredLightingVertexShader || !m_DeferredLightingFragmentShader)
		{
			DeferredLightingUberShaderBuildResult buildResult =
				DeferredLightingUberShaderBuilder::BuildForProject(
					AssetManager::GetInstance(),
					*m_ResourceFactory);
			if (!buildResult.Diagnostics.empty())
			{
				KITA_CORE_WARN(buildResult.Diagnostics);
			}
			if (buildResult.Success && buildResult.ShaderBundle.IsValid())
			{
				m_DeferredLightingVertexShader = buildResult.ShaderBundle.VertexShader;
				m_DeferredLightingFragmentShader = buildResult.ShaderBundle.FragmentShader;
			}
		}

		if (!m_PreviewSkyboxMaterial)
		{
			GetOrCreatePreviewSkyboxMaterial();
		}

		return m_SphereGeometry != nullptr;
	}

	bool PreviewSceneRenderer::EnsureOffscreenTargets(uint32_t width, uint32_t height)
	{
		width = std::max(1u, width);
		height = std::max(1u, height);
		if (m_OffscreenTargets.GBuffer &&
			m_OffscreenTargets.Lighting &&
			m_OffscreenTargets.Width == width &&
			m_OffscreenTargets.Height == height)
		{
			return true;
		}

		// 预览窗口缩放时会频繁重建离屏 RT，这里先等待 GPU 空闲，
		// 避免上一帧仍在使用旧 RT 时被直接销毁导致崩溃。
		if (m_Context &&
			(m_OffscreenTargets.GBuffer || m_OffscreenTargets.Lighting))
		{
			m_Context->WaitIdle();
		}

		// 这些 pass 持有与 render target 相关的描述，
		// RT 重建后统一跟着重建，避免继续复用旧目标状态。
		m_BasePass.reset();
		m_DeferredLightingPass.reset();
		m_ForwardOpaquePass.reset();
		m_TonemapPass.reset();
		m_SkyboxPass.reset();

		m_OffscreenTargets.Width = width;
		m_OffscreenTargets.Height = height;
		m_OffscreenTargets.GBuffer = CreateUnique<VulkanRenderTarget>(
			*m_Context,
			BuildGBufferTargetCreateInfo(width, height));
		m_OffscreenTargets.Lighting = CreateUnique<VulkanRenderTarget>(
			*m_Context,
			BuildLightingTargetCreateInfo(width, height));
		return m_OffscreenTargets.GBuffer != nullptr && m_OffscreenTargets.Lighting != nullptr;
	}

	Ref<VulkanMaterial> PreviewSceneRenderer::BuildTransientCubemapPreviewMaterial(
		AssetHandle textureHandle,
		const Ref<VulkanTexture>& texture)
	{
		if (!texture || !texture->IsValid() || texture->GetType() != TextureType::TextureCube)
		{
			return nullptr;
		}

		auto existing = m_CubemapPreviewMaterials.find(textureHandle);
		if (existing != m_CubemapPreviewMaterials.end() && existing->second)
		{
			return existing->second;
		}

		const AssetHandle shaderLabHandle = ResolveCubemapPreviewShaderLabHandle();
		if (!Asset::IsValidHandle(shaderLabHandle))
		{
			return nullptr;
		}

		MaterialAsset previewMaterialAsset{};
		previewMaterialAsset.MaterialDefinitionHandle = shaderLabHandle;

		MaterialPropertyValue baseColor{};
		baseColor.ValueType = MaterialValueType::Color;
		baseColor.Data = glm::vec4(1.0f);
		previewMaterialAsset.PropertyBlock.Values["_BaseColor"] = baseColor;

		MaterialPropertyValue cubemapValue{};
		cubemapValue.ValueType = MaterialValueType::TextureCube;
		cubemapValue.Data = textureHandle;
		previewMaterialAsset.PropertyBlock.Values["_Albedo"] = cubemapValue;

		Ref<VulkanMaterial> runtimeMaterial = m_ResourceFactory->CreateMaterial(previewMaterialAsset);
		if (!runtimeMaterial)
		{
			return nullptr;
		}

		runtimeMaterial->SetResolvedTexture("_Albedo", texture);
		runtimeMaterial->SetAlbedoTexture(texture);
		m_CubemapPreviewMaterials[textureHandle] = runtimeMaterial;
		return runtimeMaterial;
	}

	Ref<VulkanMaterial> PreviewSceneRenderer::BuildTransientSkyboxMaterial(
		const Ref<VulkanTexture>& cubemapTexture)
	{
		const AssetHandle skyboxShaderHandle = ResolveShaderLabHandle(kSkyboxShaderLabPath);
		if (!Asset::IsValidHandle(skyboxShaderHandle))
		{
			return nullptr;
		}

		MaterialAsset materialAsset{};
		materialAsset.MaterialDefinitionHandle = skyboxShaderHandle;

		Ref<VulkanMaterial> runtimeMaterial = m_ResourceFactory->CreateMaterial(materialAsset);
		if (!runtimeMaterial)
		{
			return nullptr;
		}

		if (cubemapTexture && cubemapTexture->IsValid())
		{
			runtimeMaterial->SetResolvedTexture("_Albedo", cubemapTexture);
			runtimeMaterial->SetAlbedoTexture(cubemapTexture);
		}
		return runtimeMaterial;
	}

	Ref<VulkanMaterial> PreviewSceneRenderer::GetOrCreatePreviewSkyboxMaterial()
	{
		if (m_PreviewSkyboxMaterial)
		{
			return m_PreviewSkyboxMaterial;
		}

		const AssetHandle skyboxMaterialHandle = EditorProjectBootstrap::GetPreLoadMaterialHandle("skybox");
		if (Asset::IsValidHandle(skyboxMaterialHandle))
		{
			Ref<MaterialAsset> skyboxMaterialAsset = AssetManager::GetInstance().GetMaterialAsset(skyboxMaterialHandle);
			if (skyboxMaterialAsset)
			{
				MaterialAsset transientMaterial = *skyboxMaterialAsset;
				m_PreviewSkyboxMaterial = m_ResourceFactory->CreateMaterial(transientMaterial);
				if (m_PreviewSkyboxMaterial)
				{
					m_DefaultSkyboxTexture = m_PreviewSkyboxMaterial->GetAlbedoTexture();
				}
			}
		}

		if (!m_PreviewSkyboxMaterial)
		{
			m_PreviewSkyboxMaterial = BuildTransientSkyboxMaterial(nullptr);
			if (m_PreviewSkyboxMaterial)
			{
				m_DefaultSkyboxTexture = m_PreviewSkyboxMaterial->GetAlbedoTexture();
			}
		}

		return m_PreviewSkyboxMaterial;
	}

	const VulkanMaterial::PassRuntime* PreviewSceneRenderer::FindMaterialPreviewPass(
		const VulkanMaterial& material,
		PassType passType) const
	{
		for (const VulkanMaterial::PassRuntime& pass : material.GetPasses())
		{
			if (pass.Type == passType)
			{
				return &pass;
			}
		}
		return nullptr;
	}

	bool PreviewSceneRenderer::HasMaterialPass(const VulkanMaterial& material, PassType passType) const
	{
		return FindMaterialPreviewPass(material, passType) != nullptr;
	}

	VulkanGraphicsPipeline* PreviewSceneRenderer::GetMaterialPipeline(
		const VulkanRenderTargetView& targetView,
		const VulkanGeometry& geometry,
		const VulkanMaterial& material,
		PassType passType)
	{
		if (!m_PipelineFactory || !m_SceneBindings)
		{
			return nullptr;
		}

		PipelineRequest request{};
		if (!FillMaterialPipelineRequest(request, targetView, geometry, material, passType))
		{
			return nullptr;
		}

		return m_PipelineFactory->GetOrCreate(request);
	}

	VulkanGraphicsPipeline* PreviewSceneRenderer::GetDeferredLightingPipeline(const VulkanRenderTargetView& targetView)
	{
		if (!m_DeferredLightingVertexShader || !m_DeferredLightingFragmentShader || !m_DeferredLightingPass || !m_SceneBindings)
		{
			return nullptr;
		}

		PipelineRequest request{};
		request.Pass = PassType::DeferredLighting;
		request.UseVertexInput = false;
		request.VertexShader = m_DeferredLightingVertexShader.get();
		request.FragmentShader = m_DeferredLightingFragmentShader.get();
		for (uint32_t i = 0; i < targetView.GetColorAttachmentCount(); ++i)
		{
			request.ColorFormats.push_back(targetView.GetColorFormat(i));
		}
		request.DepthFormat = targetView.HasDepthAttachment() ? targetView.GetDepthFormat() : VK_FORMAT_UNDEFINED;
		request.Samples = targetView.GetSamples();
		request.DescriptorSetLayouts = {
			m_SceneBindings->GetDescriptorSet(0).GetLayout(),
			m_DeferredLightingPass->GetDescriptorSet(0).GetLayout()
		};
		request.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		request.PolygonMode = VK_POLYGON_MODE_FILL;
		request.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		request.PushConstantStages = 0;
		request.PushConstantSize = 0;
		request.CullMode = VK_CULL_MODE_NONE;
		request.EnableDepthTest = true;
		request.EnableDepthWrite = true;
		request.DepthCompareOp = VK_COMPARE_OP_ALWAYS;
		request.EnableBlending = false;
		return m_PipelineFactory->GetOrCreate(request);
	}

	VulkanGraphicsPipeline* PreviewSceneRenderer::GetTonemapPipeline(const VulkanRenderTargetView& targetView)
	{
		if (!m_TonemapVertexShader || !m_TonemapFragmentShader || !m_TonemapPass || !m_SceneBindings)
		{
			return nullptr;
		}

		PipelineRequest request{};
		request.Pass = PassType::PostProcess;
		request.UseVertexInput = false;
		request.VertexShader = m_TonemapVertexShader.get();
		request.FragmentShader = m_TonemapFragmentShader.get();
		for (uint32_t i = 0; i < targetView.GetColorAttachmentCount(); ++i)
		{
			request.ColorFormats.push_back(targetView.GetColorFormat(i));
		}
		request.DepthFormat = targetView.HasDepthAttachment() ? targetView.GetDepthFormat() : VK_FORMAT_UNDEFINED;
		request.Samples = targetView.GetSamples();
		request.DescriptorSetLayouts = {
			m_SceneBindings->GetDescriptorSet(0).GetLayout(),
			m_TonemapPass->GetDescriptorSet(0).GetLayout()
		};
		request.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		request.PolygonMode = VK_POLYGON_MODE_FILL;
		request.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		request.PushConstantStages = 0;
		request.PushConstantSize = 0;
		request.CullMode = VK_CULL_MODE_NONE;
		request.EnableDepthTest = false;
		request.EnableDepthWrite = false;
		request.DepthCompareOp = VK_COMPARE_OP_ALWAYS;
		request.EnableBlending = false;
		return m_PipelineFactory->GetOrCreate(request);
	}

	VulkanGraphicsPipeline* PreviewSceneRenderer::GetSkyboxPipeline(
		const VulkanRenderTargetView& targetView,
		const VulkanMaterial& skyboxMaterial)
	{
		if (!m_PipelineFactory || !m_SceneBindings)
		{
			return nullptr;
		}

		const VulkanMaterial::PassRuntime* skyboxPass = skyboxMaterial.FindPass(PassType::PostProcess);
		if (!skyboxPass || !skyboxPass->VertexShader || !skyboxPass->FragmentShader)
		{
			return nullptr;
		}

		PipelineRequest request{};
		request.Pass = PassType::PostProcess;
		request.UseVertexInput = false;
		request.VertexShader = skyboxPass->VertexShader.get();
		request.FragmentShader = skyboxPass->FragmentShader.get();
		for (uint32_t i = 0; i < targetView.GetColorAttachmentCount(); ++i)
		{
			request.ColorFormats.push_back(targetView.GetColorFormat(i));
		}
		request.DepthFormat = targetView.HasDepthAttachment() ? targetView.GetDepthFormat() : VK_FORMAT_UNDEFINED;
		request.Samples = targetView.GetSamples();
		request.DescriptorSetLayouts = {
			m_SceneBindings->GetDescriptorSet(0).GetLayout(),
			skyboxMaterial.GetDescriptorSet(0).GetLayout()
		};
		request.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		request.PolygonMode = VK_POLYGON_MODE_FILL;
		request.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		request.PushConstantStages = VK_SHADER_STAGE_FRAGMENT_BIT;
		request.PushConstantSize = SkyboxPushConstantSize;
		request.CullMode = skyboxPass->RenderState.CullMode;
		request.EnableDepthTest = skyboxPass->RenderState.DepthTest;
		request.EnableDepthWrite = skyboxPass->RenderState.DepthWrite;
		request.DepthCompareOp = skyboxPass->RenderState.DepthCompareOp;
		request.EnableBlending = skyboxPass->RenderState.Blend;
		return m_PipelineFactory->GetOrCreate(request);
	}

	bool PreviewSceneRenderer::FillMaterialPipelineRequest(
		PipelineRequest& request,
		const VulkanRenderTargetView& targetView,
		const VulkanGeometry& geometry,
		const VulkanMaterial& material,
		PassType passType) const
	{
		const VulkanMaterial::PassRuntime* materialPass = FindMaterialPreviewPass(material, passType);
		if (!materialPass || !materialPass->VertexShader || !materialPass->FragmentShader || !m_SceneBindings)
		{
			return false;
		}

		request = {};
		request.Pass = passType;
		request.Geometry = &geometry;
		request.VertexShader = materialPass->VertexShader.get();
		request.FragmentShader = materialPass->FragmentShader.get();
		for (uint32_t i = 0; i < targetView.GetColorAttachmentCount(); ++i)
		{
			request.ColorFormats.push_back(targetView.GetColorFormat(i));
		}
		request.DepthFormat = targetView.HasDepthAttachment() ? targetView.GetDepthFormat() : VK_FORMAT_UNDEFINED;
		request.Samples = targetView.GetSamples();
		request.DescriptorSetLayouts = {
			m_SceneBindings->GetDescriptorSet(0).GetLayout(),
			material.GetDescriptorSet(0).GetLayout()
		};

		const bool useLegacyDefaults = material.GetRuntimeLayout() == nullptr;
		ApplyMaterialRenderState(request, *materialPass, useLegacyDefaults);
		return true;
	}

	void PreviewSceneRenderer::ApplyMaterialRenderState(
		PipelineRequest& request,
		const VulkanMaterial::PassRuntime& materialPass,
		bool useLegacyDefaults) const
	{
		request.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		request.PolygonMode = VK_POLYGON_MODE_FILL;
		request.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		request.PushConstantStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		request.PushConstantSize = ObjectDataSize;

		if (useLegacyDefaults)
		{
			request.CullMode = VK_CULL_MODE_NONE;
			request.EnableDepthTest = true;
			request.EnableDepthWrite = true;
			request.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
			request.EnableBlending = false;
			return;
		}

		request.CullMode = materialPass.RenderState.CullMode;
		request.EnableDepthTest = materialPass.RenderState.DepthTest;
		request.EnableDepthWrite = materialPass.RenderState.DepthWrite;
		request.DepthCompareOp = materialPass.RenderState.DepthCompareOp;
		request.EnableBlending = materialPass.RenderState.Blend;
	}

	ScenePassData PreviewSceneRenderer::BuildSceneData(
		const PreviewOrbitCameraState& cameraState,
		uint32_t width,
		uint32_t height,
		const RenderPassBeginInfo& beginInfo) const
	{
		const glm::vec3 eye = BuildCameraPosition(cameraState);
		const glm::vec3 center = cameraState.Target;
		const glm::vec3 up(0.0f, 1.0f, 0.0f);
		const float aspect = static_cast<float>(std::max(1u, width)) / static_cast<float>(std::max(1u, height));

		ScenePassData sceneData{};
		sceneData.Camera.Matrix_V = glm::lookAtRH(eye, center, up);
		sceneData.Camera.Matrix_P = glm::perspectiveRH_ZO(glm::radians(45.0f), aspect, 0.1f, 32.0f);
		sceneData.Camera.Matrix_VP = sceneData.Camera.Matrix_P * sceneData.Camera.Matrix_V;
		sceneData.Camera.Matrix_I_V = glm::inverse(sceneData.Camera.Matrix_V);
		sceneData.Camera.Matrix_I_P = glm::inverse(sceneData.Camera.Matrix_P);
		sceneData.Camera.Matrix_I_VP = glm::inverse(sceneData.Camera.Matrix_VP);
		sceneData.Camera.CameraPosWS = glm::vec4(eye, 1.0f);

		sceneData.MainLight.Direction = glm::vec4(glm::normalize(glm::vec3(-0.35f, -0.55f, -0.75f)), 0.0f);
		sceneData.MainLight.Color = glm::vec4(1.0f, 0.96f, 0.9f, 1.0f);
		sceneData.BeginInfo = beginInfo;
		return sceneData;
	}

	ObjectData PreviewSceneRenderer::BuildPreviewObjectData() const
	{
		ObjectData objectData{};
		const glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.72f));
		objectData.Matrix_M = model;
		objectData.Matrix_I_M = glm::inverse(model);
		return objectData;
	}

	AssetHandle PreviewSceneRenderer::ResolveShaderLabHandle(const std::filesystem::path& relativePath)
	{
		AssetManager& assetManager = AssetManager::GetInstance();
		AssetHandle handle = assetManager.GetHandleByPath(relativePath);
		if (Asset::IsValidHandle(handle))
		{
			return handle;
		}

		const Ref<Project> project = Project::GetActive();
		if (!project)
		{
			return InvalidAssetHandle;
		}

		return assetManager.ImportAsset(project->GetAssetRootDirectory() / relativePath);
	}

	AssetHandle PreviewSceneRenderer::ResolveCubemapPreviewShaderLabHandle()
	{
		if (!Asset::IsValidHandle(m_CubemapPreviewShaderLabHandle))
		{
			m_CubemapPreviewShaderLabHandle = ResolveShaderLabHandle(kCubemapPreviewShaderLabPath);
		}
		return m_CubemapPreviewShaderLabHandle;
	}

	VulkanRenderTarget::CreateInfo PreviewSceneRenderer::BuildGBufferTargetCreateInfo(uint32_t width, uint32_t height)
	{
		VulkanRenderTarget::CreateInfo createInfo{};
		createInfo.Name = "PreviewScene_GBuffer";
		createInfo.Width = std::max(1u, width);
		createInfo.Height = std::max(1u, height);
		createInfo.Samples = VK_SAMPLE_COUNT_1_BIT;

		createInfo.ColorAttachments = {
			{"Preview_GBuffer0", VK_FORMAT_R8G8B8A8_UNORM},
			{"Preview_GBuffer1", VK_FORMAT_R8G8B8A8_UNORM},
			{"Preview_GBuffer2", VK_FORMAT_R16G16B16A16_SFLOAT},
			{"Preview_GBuffer3", VK_FORMAT_R16G16B16A16_SFLOAT},
			{"Preview_GBuffer4", VK_FORMAT_R16G16B16A16_SFLOAT}
		};
		createInfo.DepthAttachment.Enabled = true;
		createInfo.DepthAttachment.Name = "Preview_GBufferDepth";
		createInfo.DepthAttachment.Format = VK_FORMAT_D32_SFLOAT;
		createInfo.DepthAttachment.CreateSampler = true;
		return createInfo;
	}

	VulkanRenderTarget::CreateInfo PreviewSceneRenderer::BuildLightingTargetCreateInfo(uint32_t width, uint32_t height)
	{
		VulkanRenderTarget::CreateInfo createInfo{};
		createInfo.Name = "PreviewScene_Lighting";
		createInfo.Width = std::max(1u, width);
		createInfo.Height = std::max(1u, height);
		createInfo.Samples = VK_SAMPLE_COUNT_1_BIT;

		VulkanRenderTarget::ColorAttachmentDesc colorAttachment{};
		colorAttachment.Name = "Preview_LightingColor";
		colorAttachment.Format = VK_FORMAT_R16G16B16A16_SFLOAT;
		colorAttachment.CreateSampler = true;
		colorAttachment.Filter = VK_FILTER_LINEAR;
		colorAttachment.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		createInfo.ColorAttachments.push_back(colorAttachment);

		createInfo.DepthAttachment.Enabled = true;
		createInfo.DepthAttachment.Name = "Preview_LightingDepth";
		createInfo.DepthAttachment.Format = VK_FORMAT_D32_SFLOAT;
		createInfo.DepthAttachment.CreateSampler = false;
		return createInfo;
	}

}
