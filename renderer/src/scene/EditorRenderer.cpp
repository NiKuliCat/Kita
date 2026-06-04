#include "renderer_pch.h"
#include "EditorRenderer.h"

#include "asset/AssetManager.h"
#include "component/MeshRenderer.h"
#include "component/Transform.h"
#include "project/EditorProjectBootstrap.h"
#include "render/deferred/DeferredLightingUberShaderBuilder.h"
#include "ui/viewport/EditorPickRegistry.h"
#include "ui/viewport/EditorViewportSurface.h"

#include <backends/imgui_impl_vulkan.h>
#include <sstream>

namespace Kita {

	namespace
	{
		const ShaderLabCompiledPass* FindShaderLabCompiledPass(
			const ShaderLabAsset& shaderLabAsset,
			PassType passType)
		{
			for (const ShaderLabCompiledPass& compiledPass : shaderLabAsset.CompiledPasses)
			{
				if (compiledPass.Type == passType)
					return &compiledPass;
			}

			return nullptr;
		}

		const char* PassTypeToString(PassType passType)
		{
			switch (passType)
			{
			case PassType::GBuffer: return "GBuffer";
			case PassType::DeferredLighting: return "DeferredLighting";
			case PassType::ShadowCaster: return "ShadowCaster";
			case PassType::DepthOnly: return "DepthOnly";
			case PassType::ForwardOpaque: return "ForwardOpaque";
			case PassType::ForwardTransparent: return "ForwardTransparent";
			case PassType::PostProcess: return "PostProcess";
			case PassType::EditorPicking: return "EditorPicking";
			case PassType::UI: return "UI";
			default: return "Unknown";
			}
		}

		struct PreloadedShaderLabPassResources
		{
			VulkanResourceFactory::ShaderBundle ShaderBundle{};
			ShaderLabRenderStateDesc RenderState{};
			PassType Type = PassType::Unknown;

			bool IsValid() const
			{
				return ShaderBundle.IsValid();
			}
		};
/*

		// 统一加载 editor 预配置的 ShaderLab pass，避免 grid / tonemap / picking 各自重复解析 pass 和 render state。
*/
		bool TryLoadPreloadedShaderLabPassResources(
			const char* preloadName,
			PassType passType,
			VulkanResourceFactory& resourceFactory,
			PreloadedShaderLabPassResources& outResources,
			std::string& outReason)
		{
			auto& assetManager = AssetManager::GetInstance();
			const AssetHandle shaderLabHandle = EditorProjectBootstrap::GetPreLoadShaderHandle(preloadName);
			if (!Asset::IsValidHandle(shaderLabHandle))
			{
				outReason = "preload shader handle '" + std::string(preloadName) + "' is invalid";
				return false;
			}

			const AssetMetadata* metadata = assetManager.GetMetadata(shaderLabHandle);
			if (!metadata)
			{
				outReason = "preload shader '" + std::string(preloadName) + "' metadata is missing";
				return false;
			}

			if (metadata->type != AssetType::ShaderLab)
			{
				outReason =
					"preload shader '" + std::string(preloadName) +
					"' must reference a ShaderLab asset in the unified runtime path";
				return false;
			}

			Ref<ShaderLabAsset> shaderLabAsset = assetManager.GetShaderLabAsset(shaderLabHandle);
			if (!shaderLabAsset)
			{
				outReason = "failed to load ShaderLab asset for preload shader '" + std::string(preloadName) + "'";
				return false;
			}

			const ShaderLabCompiledPass* compiledPass = FindShaderLabCompiledPass(*shaderLabAsset, passType);
			if (!compiledPass)
			{
				outReason =
					"ShaderLab asset '" + shaderLabAsset->SourcePath.generic_string() +
					"' has no pass for type '" + std::string(PassTypeToString(passType)) + "'";
				return false;
			}

			VulkanResourceFactory::ShaderBundle shaderBundle =
				resourceFactory.BuildShaderLabPassBundle(shaderLabHandle, passType);
			if (!shaderBundle.IsValid())
			{
				outReason =
					"failed to build ShaderLab pass bundle for '" +
					shaderLabAsset->SourcePath.generic_string() + "'";
				return false;
			}

			outResources.ShaderBundle = std::move(shaderBundle);
			outResources.RenderState = compiledPass->RenderState;
			outResources.Type = compiledPass->Type;
			return true;
		}

		void ApplyShaderLabRenderState(
			PipelineRequest& request,
			const ShaderLabRenderStateDesc& renderState)
		{
			request.CullMode = renderState.CullMode;
			request.EnableDepthTest = renderState.DepthTest;
			request.EnableDepthWrite = renderState.DepthWrite;
			request.DepthCompareOp = renderState.DepthCompareOp;
			request.EnableBlending = renderState.Blend;
		}

		const char* MaterialValueTypeToString(MaterialValueType valueType)
		{
			switch (valueType)
			{
			case MaterialValueType::Bool: return "Bool";
			case MaterialValueType::Int: return "Int";
			case MaterialValueType::Float: return "Float";
			case MaterialValueType::Float2: return "Float2";
			case MaterialValueType::Float3: return "Float3";
			case MaterialValueType::Float4: return "Float4";
			case MaterialValueType::Color: return "Color";
			case MaterialValueType::Texture2D: return "Texture2D";
			case MaterialValueType::TextureCube: return "TextureCube";
			default: return "Unknown";
			}
		}

		const char* CullModeToString(VkCullModeFlags cullMode)
		{
			switch (cullMode)
			{
			case VK_CULL_MODE_NONE: return "None";
			case VK_CULL_MODE_FRONT_BIT: return "Front";
			case VK_CULL_MODE_BACK_BIT: return "Back";
			case VK_CULL_MODE_FRONT_AND_BACK: return "FrontAndBack";
			default: return "Unknown";
			}
		}

		const char* CompareOpToString(VkCompareOp compareOp)
		{
			switch (compareOp)
			{
			case VK_COMPARE_OP_NEVER: return "Never";
			case VK_COMPARE_OP_LESS: return "Less";
			case VK_COMPARE_OP_EQUAL: return "Equal";
			case VK_COMPARE_OP_LESS_OR_EQUAL: return "LessEqual";
			case VK_COMPARE_OP_GREATER: return "Greater";
			case VK_COMPARE_OP_NOT_EQUAL: return "NotEqual";
			case VK_COMPARE_OP_GREATER_OR_EQUAL: return "GreaterEqual";
			case VK_COMPARE_OP_ALWAYS: return "Always";
			default: return "Unknown";
			}
		}

		static std::string FormatFloatList(std::initializer_list<float> values)
		{
			std::ostringstream oss;
			oss << "(";
			size_t index = 0;
			for (float value : values)
			{
				if (index++ > 0)
					oss << ", ";
				oss << value;
			}
			oss << ")";
			return oss.str();
		}

		static std::string MaterialPropertyValueToString(const MaterialPropertyValue& value)
		{
			switch (value.ValueType)
			{
			case MaterialValueType::Bool:
				return std::get<bool>(value.Data) ? "true" : "false";
			case MaterialValueType::Int:
				return std::to_string(std::get<int32_t>(value.Data));
			case MaterialValueType::Float:
				return std::to_string(std::get<float>(value.Data));
			case MaterialValueType::Float2:
			{
				const glm::vec2 vec = std::get<glm::vec2>(value.Data);
				return FormatFloatList({ vec.x, vec.y });
			}
			case MaterialValueType::Float3:
			{
				const glm::vec3 vec = std::get<glm::vec3>(value.Data);
				return FormatFloatList({ vec.x, vec.y, vec.z });
			}
			case MaterialValueType::Float4:
			case MaterialValueType::Color:
			{
				const glm::vec4 vec = std::get<glm::vec4>(value.Data);
				return FormatFloatList({ vec.x, vec.y, vec.z, vec.w });
			}
			case MaterialValueType::Texture2D:
			case MaterialValueType::TextureCube:
				return std::to_string(static_cast<uint64_t>(std::get<AssetHandle>(value.Data)));
			default:
				return "<unsupported>";
			}
		}
	}

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

		m_ForwardOpaquePass = CreateUnique<ForwardOpaquePass>(
			m_SceneBindings,
			MakeRenderPassDesc(m_LightingTargetDesc, "ForwardOpaquePass", PassType::ForwardOpaque)
		);

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
		InitPickingResources();
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

		if (gbuffer.HasDepth())
			gbufferPass.WriteDepth(gbuffer.DepthAttachment);

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
			KITA_CORE_ASSERT(gbuffer.ColorAttachments.size() >= 5, "DeferredLighting Uber shader requires 5 GBuffer color attachments");
			KITA_CORE_ASSERT(gbuffer.HasDepth(), "Current DeferredLighting shader requires GBuffer depth");


			m_RenderGraph->AddPass("Lighting")
				.ReadTexture(gbuffer.ColorAttachments[0])
				.ReadTexture(gbuffer.ColorAttachments[1])
				.ReadTexture(gbuffer.ColorAttachments[2])
				.ReadTexture(gbuffer.ColorAttachments[3])
				.ReadTexture(gbuffer.ColorAttachments[4])
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

		if (m_ForwardOpaquePass)
		{
			m_RenderGraph->AddPass("ForwardOpaque")
				.ReadTexture(lightingColor)
				.ReadDepth(lightingDepth)
				.WriteColor(lightingColor)
				.WriteDepth(lightingDepth)
				.SetExecute([this, lightingColor, lightingDepth](RenderGraphContext& graphContext)
					{
						VulkanRenderTargetView lightingRt = graphContext.BuildRenderTargetView(
							"Lighting.ForwardOpaque",
							{ lightingColor },
							&lightingDepth);

						RenderPassContext passContext(
							graphContext.GetVulkanContext(),
							graphContext.GetCommandBuffer(),
							lightingRt);

						m_ForwardOpaquePass->Execute(passContext);
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
		m_PickingVertexShader.reset();
		m_PickingFragmentShader.reset();

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
		if (!m_Context || !m_EditorGridPass || !m_VulkanResFactory)
			return;

		PreloadedShaderLabPassResources resources{};
		std::string failureReason;
		if (!TryLoadPreloadedShaderLabPassResources(
			"grid",
			PassType::PostProcess,
			*m_VulkanResFactory,
			resources,
			failureReason))
		{
			KITA_CORE_WARN("EditorRenderer: {}", failureReason);
			return;
		}

		m_GridVertexShader = resources.ShaderBundle.VertexShader;
		m_GridFragmentShader = resources.ShaderBundle.FragmentShader;
		m_GridRenderState = resources.RenderState;
	}

	void EditorRenderer::InitDeferredLightingResources()
	{
		if (!m_Context || !m_DeferredLightingPass)
			return;

		const std::filesystem::path deprecatedDeferredLightingPath =
			EditorProjectBootstrap::GetPreLoadShaderPath("deferredLighting");
		if (!deprecatedDeferredLightingPath.empty())
		{
			KITA_CORE_WARN(
				"EditorRenderer: config 'asset.shader.deferredLighting' is deprecated and ignored in the Uber deferred lighting path. configured='{}'.",
				deprecatedDeferredLightingPath.generic_string());
		}

		DeferredLightingUberShaderBuildResult buildResult =
			DeferredLightingUberShaderBuilder::BuildForProject(
				AssetManager::GetInstance(),
				*m_VulkanResFactory);

		if (!buildResult.Diagnostics.empty())
		{
			KITA_CORE_WARN(buildResult.Diagnostics);
		}

		if (!buildResult.Success || !buildResult.ShaderBundle.IsValid())
		{
			KITA_CORE_ERROR("EditorRenderer: failed to build deferred lighting Uber shader.");
			return;
		}

		if (!m_LoggedDeferredLightingInit)
		{
			m_LoggedDeferredLightingInit = true;
			std::ostringstream diagnostic;
			diagnostic
				<< "EditorRenderer deferred lighting diagnostic: source=UberShader"
				<< ", surfaceShaders=" << buildResult.SurfaceShaderCount
				<< ", customShadingModels=" << buildResult.CustomShadingModelCount
				<< ", VS='" << buildResult.ShaderBundle.VertexShader->GetName()
				<< "', FS='" << buildResult.ShaderBundle.FragmentShader->GetName()
				<< "', state={Cull=" << CullModeToString(VK_CULL_MODE_NONE)
				<< ", DepthTest=On"
				<< ", DepthWrite=On"
				<< ", DepthOp=" << CompareOpToString(VK_COMPARE_OP_ALWAYS)
				<< ", Blend=Off}";
			KITA_CORE_WARN(diagnostic.str());
		}

		m_DeferredLightingVertexShader = buildResult.ShaderBundle.VertexShader;
		m_DeferredLightingFragmentShader = buildResult.ShaderBundle.FragmentShader;
	}

	void EditorRenderer::InitTonemapResources()
	{
		if (!m_Context || !m_TonemapPass || !m_VulkanResFactory)
			return;

		PreloadedShaderLabPassResources resources{};
		std::string failureReason;
		if (!TryLoadPreloadedShaderLabPassResources(
			"tonemap",
			PassType::PostProcess,
			*m_VulkanResFactory,
			resources,
			failureReason))
		{
			KITA_CORE_WARN("EditorRenderer: {}", failureReason);
			return;
		}

		m_TonemapVertexShader = resources.ShaderBundle.VertexShader;
		m_TonemapFragmentShader = resources.ShaderBundle.FragmentShader;
		m_TonemapRenderState = resources.RenderState;
	}

	void EditorRenderer::InitPickingResources()
	{
		if (!m_Context || !m_ViewportPickingPass || !m_VulkanResFactory)
			return;

		PreloadedShaderLabPassResources resources{};
		std::string failureReason;
		if (!TryLoadPreloadedShaderLabPassResources(
			"picking",
			PassType::EditorPicking,
			*m_VulkanResFactory,
			resources,
			failureReason))
		{
			KITA_CORE_WARN("EditorRenderer: {}", failureReason);
			return;
		}

		m_PickingVertexShader = resources.ShaderBundle.VertexShader;
		m_PickingFragmentShader = resources.ShaderBundle.FragmentShader;
		m_PickingRenderState = resources.RenderState;
	}

	const VulkanMaterial::PassRuntime* EditorRenderer::FindMaterialPassForScene(
		const VulkanMaterial& material,
		PassType passType) const
	{
		const bool useRuntimePasses =
			material.GetRuntimeLayout() != nullptr &&
			!material.GetPasses().empty();
		if (!useRuntimePasses)
		{
			return passType == PassType::GBuffer
				? material.FindPass(PassType::GBuffer)
				: nullptr;
		}

		if (const VulkanMaterial::PassRuntime* exactPass = material.FindPass(passType))
		{
			if (exactPass->Type == passType)
				return exactPass;
		}

		if (passType == PassType::ShadowCaster)
		{
			if (const VulkanMaterial::PassRuntime* depthOnlyPass = material.FindPass(PassType::DepthOnly))
			{
				if (depthOnlyPass->Type == PassType::DepthOnly)
					return depthOnlyPass;
			}
		}

		if (passType == PassType::DepthOnly)
		{
			if (const VulkanMaterial::PassRuntime* shadowPass = material.FindPass(PassType::ShadowCaster))
			{
				if (shadowPass->Type == PassType::ShadowCaster)
					return shadowPass;
			}
		}

		return nullptr;
	}

	void EditorRenderer::LogMaterialDiagnosticOnce(
		AssetHandle materialHandle,
		const std::string& objectName,
		PassType passType,
		const std::string& reason,
		const VulkanMaterial* runtimeMaterial) const
	{
		std::ostringstream keyStream;
		keyStream
			<< static_cast<uint64_t>(materialHandle)
			<< "|" << PassTypeToString(passType)
			<< "|" << reason;
		const std::string key = keyStream.str();
		if (!m_LoggedMaterialDiagnosticKeys.insert(key).second)
		{
			return;
		}

		std::ostringstream message;
		message
			<< "EditorRenderer material diagnostic: object='"
			<< objectName
			<< "', material="
			<< BuildMaterialDiagnosticLabel(materialHandle)
			<< ", pass="
			<< PassTypeToString(passType)
			<< ", reason="
			<< reason;

		const std::string shaderLabDiagnostic = BuildShaderLabPropertyDiagnostic(materialHandle, runtimeMaterial);
		if (!shaderLabDiagnostic.empty())
		{
			message << ", " << shaderLabDiagnostic;
		}

		if (runtimeMaterial)
		{
			message << ", runtimePasses=" << BuildRuntimePassDiagnostic(*runtimeMaterial);
			const std::string textureDiagnostic = BuildResolvedTextureDiagnostic(materialHandle, *runtimeMaterial);
			if (!textureDiagnostic.empty())
			{
				message << ", " << textureDiagnostic;
			}
		}

		KITA_CORE_WARN(message.str());
	}

	void EditorRenderer::LogMaterialQueueSuccessOnce(
		AssetHandle materialHandle,
		const std::string& objectName,
		PassType passType,
		const VulkanGraphicsPipeline& pipeline,
		const VulkanMaterial* runtimeMaterial) const
	{
		const std::string reason = std::string("queued successfully with pipeline '") + pipeline.GetName() + "'";
		LogMaterialDiagnosticOnce(materialHandle, objectName, passType, reason, runtimeMaterial);
	}

	std::string EditorRenderer::BuildMaterialDiagnosticLabel(AssetHandle materialHandle) const
	{
		std::ostringstream oss;
		oss << "handle=" << static_cast<uint64_t>(materialHandle);

		if (const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(materialHandle))
		{
			oss << " path='" << metadata->relativePath.generic_string() << "'";
		}

		return oss.str();
	}

	std::string EditorRenderer::BuildShaderLabPropertyDiagnostic(
		AssetHandle materialHandle,
		const VulkanMaterial* runtimeMaterial) const
	{
		Ref<MaterialAsset> materialAsset = AssetManager::GetInstance().GetMaterialAsset(materialHandle);
		if (!materialAsset || !Asset::IsValidHandle(materialAsset->ShaderLabHandle))
		{
			return {};
		}

		Ref<ShaderLabAsset> shaderLabAsset = AssetManager::GetInstance().GetShaderLabAsset(materialAsset->ShaderLabHandle);
		if (!shaderLabAsset || !shaderLabAsset->Desc)
		{
			return "shaderLabAsset=unavailable";
		}

		std::vector<std::string> missingProperties;
		std::vector<std::string> typeMismatches;
		for (const MaterialPropertyDesc& property : shaderLabAsset->Desc->Properties)
		{
			auto valueIt = materialAsset->PropertyBlock.Values.find(property.Name);
			if (valueIt == materialAsset->PropertyBlock.Values.end())
			{
				missingProperties.push_back(property.Name);
				continue;
			}

			if (valueIt->second.ValueType != property.ValueType)
			{
				std::ostringstream mismatch;
				mismatch
					<< property.Name
					<< "(expected=" << MaterialValueTypeToString(property.ValueType)
					<< ", actual=" << MaterialValueTypeToString(valueIt->second.ValueType)
					<< ")";
				typeMismatches.push_back(mismatch.str());
			}
		}

		std::ostringstream oss;
		oss << "shaderLab=" << BuildMaterialDiagnosticLabel(materialAsset->ShaderLabHandle);
		if (missingProperties.empty() && typeMismatches.empty())
		{
			oss << " properties=ok";
		}
		else
		{
			if (!missingProperties.empty())
			{
				oss << " missing=[";
				for (size_t i = 0; i < missingProperties.size(); ++i)
				{
					if (i > 0)
						oss << ", ";
					oss << missingProperties[i];
				}
				oss << "]";
			}

			if (!typeMismatches.empty())
			{
				oss << " typeMismatch=[";
				for (size_t i = 0; i < typeMismatches.size(); ++i)
				{
					if (i > 0)
						oss << ", ";
					oss << typeMismatches[i];
				}
				oss << "]";
			}
		}

		// 一次性把运行时 UBO 成员 offset 和最终会落进去的值打印出来，方便核对 ShaderLab packing。
		if (shaderLabAsset->RuntimeLayout)
		{
			oss << " uboLayout=[";
			for (size_t i = 0; i < shaderLabAsset->RuntimeLayout->UniformMembers.size(); ++i)
			{
				const MaterialUniformMember& member = shaderLabAsset->RuntimeLayout->UniformMembers[i];
				if (i > 0)
					oss << ", ";
				oss << member.Name << "@" << member.Offset << "/" << member.Size;
			}
			oss << "]";

			oss << " uboValues=[";
			for (size_t i = 0; i < shaderLabAsset->RuntimeLayout->UniformMembers.size(); ++i)
			{
				const MaterialUniformMember& member = shaderLabAsset->RuntimeLayout->UniformMembers[i];
				if (i > 0)
					oss << ", ";

				const MaterialPropertyDesc* propertyDesc = nullptr;
				for (const MaterialPropertyDesc& property : shaderLabAsset->Desc->Properties)
				{
					if (property.Name == member.Name)
					{
						propertyDesc = &property;
						break;
					}
				}

				const MaterialPropertyValue* valueToLog = nullptr;
				auto valueIt = materialAsset->PropertyBlock.Values.find(member.Name);
				if (valueIt != materialAsset->PropertyBlock.Values.end() && valueIt->second.ValueType == member.ValueType)
				{
					valueToLog = &valueIt->second;
				}
				else if (propertyDesc && propertyDesc->DefaultValue.ValueType == member.ValueType)
				{
					valueToLog = &propertyDesc->DefaultValue;
				}

				oss << member.Name << "=";
				if (member.Name == MaterialSystemFieldShadingModelID && runtimeMaterial)
				{
					oss << runtimeMaterial->GetLightingRuntime().ShadingModelID;
				}
				else if (member.Name == MaterialSystemFieldCustomDataCount && runtimeMaterial)
				{
					oss << runtimeMaterial->GetLightingRuntime().CustomDataCount;
				}
				else if (valueToLog)
				{
					oss << MaterialPropertyValueToString(*valueToLog);
				}
				else
				{
					oss << "<missing>";
				}
			}
			oss << "]";
		}

		return oss.str();
	}

	std::string EditorRenderer::BuildResolvedTextureDiagnostic(
		AssetHandle materialHandle,
		const VulkanMaterial& material) const
	{
		const MaterialRuntimeLayout* runtimeLayout = material.GetRuntimeLayout();
		if (!runtimeLayout || runtimeLayout->ResourceBindings.empty())
		{
			return {};
		}

		Ref<MaterialAsset> materialAsset = AssetManager::GetInstance().GetMaterialAsset(materialHandle);
		if (!materialAsset)
		{
			return {};
		}

		Ref<ShaderLabAsset> shaderLabAsset = AssetManager::GetInstance().GetShaderLabAsset(materialAsset->ShaderLabHandle);
		if (!shaderLabAsset || !shaderLabAsset->Desc)
		{
			return {};
		}

		std::ostringstream oss;
		oss << "textures=[";
		for (size_t i = 0; i < runtimeLayout->ResourceBindings.size(); ++i)
		{
			const MaterialResourceBinding& binding = runtimeLayout->ResourceBindings[i];
			if (i > 0)
				oss << ", ";

			oss << binding.Name << "->";

			const MaterialPropertyDesc* propertyDesc = nullptr;
			for (const MaterialPropertyDesc& property : shaderLabAsset->Desc->Properties)
			{
				if (property.Name == binding.Name)
				{
					propertyDesc = &property;
					break;
				}
			}

			const MaterialPropertyValue* propertyValue = nullptr;
			auto valueIt = materialAsset->PropertyBlock.Values.find(binding.Name);
			if (valueIt != materialAsset->PropertyBlock.Values.end() &&
				valueIt->second.ValueType == binding.ValueType)
			{
				propertyValue = &valueIt->second;
			}
			else if (propertyDesc && propertyDesc->DefaultValue.ValueType == binding.ValueType)
			{
				propertyValue = &propertyDesc->DefaultValue;
			}

			if (propertyValue && std::holds_alternative<AssetHandle>(propertyValue->Data))
			{
				const AssetHandle textureHandle = std::get<AssetHandle>(propertyValue->Data);
				oss << "asset=" << static_cast<uint64_t>(textureHandle);
				if (const AssetMetadata* metadata = AssetManager::GetInstance().GetMetadata(textureHandle))
				{
					oss << " path='" << metadata->relativePath.generic_string() << "'";
				}
			}
			else if (propertyValue && std::holds_alternative<std::string>(propertyValue->Data))
			{
				oss << "default='" << std::get<std::string>(propertyValue->Data) << "'";
			}
			else
			{
				oss << "source=<fallback>";
			}

			const Ref<VulkanTexture>& resolvedTexture = material.GetResolvedTexture(binding.Name);
			if (resolvedTexture && resolvedTexture->IsValid())
			{
				oss << " runtime='" << resolvedTexture->GetName() << "'";
			}
			else
			{
				oss << " runtime=<fallback>";
			}
		}
		oss << "]";
		return oss.str();
	}

	std::string EditorRenderer::BuildRuntimePassDiagnostic(const VulkanMaterial& material) const
	{
		if (material.GetPasses().empty())
		{
			return "<none>";
		}

		std::ostringstream oss;
		for (size_t i = 0; i < material.GetPasses().size(); ++i)
		{
			const VulkanMaterial::PassRuntime& pass = material.GetPasses()[i];
			if (i > 0)
			{
				oss << "; ";
			}

			oss
				<< pass.Name
				<< "(" << PassTypeToString(pass.Type)
				<< ", VS=" << (pass.VertexShader ? "yes" : "no")
				<< ", FS=" << (pass.FragmentShader ? "yes" : "no")
				<< ", Cull=" << CullModeToString(pass.RenderState.CullMode)
				<< ", DepthTest=" << (pass.RenderState.DepthTest ? "On" : "Off")
				<< ", DepthWrite=" << (pass.RenderState.DepthWrite ? "On" : "Off")
				<< ", DepthOp=" << CompareOpToString(pass.RenderState.DepthCompareOp)
				<< ", Blend=" << (pass.RenderState.Blend ? "On" : "Off")
				<< ")";
		}

		return oss.str();
	}

	bool EditorRenderer::FillMaterialPipelineRequest(
		PipelineRequest& request,
		const RenderGraphTransientRenderTargetDesc& targetDesc,
		const VulkanGeometry& geometry,
		const VulkanMaterial& material,
		PassType passType,
		std::string* failureReason) const
	{
		const VulkanMaterial::PassRuntime* materialPass = FindMaterialPassForScene(material, passType);
		if (!materialPass)
		{
			if (failureReason)
				*failureReason = "material pass not found for scene";
			return false;
		}

		if (!materialPass->VertexShader || !materialPass->FragmentShader)
		{
			if (failureReason)
			{
				*failureReason = std::string("material pass is missing shader stage: VS=") +
					(materialPass->VertexShader ? "yes" : "no") +
					", FS=" +
					(materialPass->FragmentShader ? "yes" : "no");
			}
			return false;
		}

		request = {};
		request.Pass = passType;
		request.Geometry = &geometry;
		request.VertexShader = materialPass->VertexShader.get();
		request.FragmentShader = materialPass->FragmentShader.get();
		request.ColorFormats.reserve(targetDesc.Colors.size());
		for (const RenderGraphColorAttachmentDesc& color : targetDesc.Colors)
		{
			request.ColorFormats.push_back(color.Format);
		}

		request.DepthFormat = targetDesc.Depth.Enabled ? targetDesc.Depth.Format : VK_FORMAT_UNDEFINED;
		request.Samples = targetDesc.Samples;
		request.DescriptorSetLayouts = {
			m_SceneBindings.GetDescriptorSet(0).GetLayout(),
			material.GetDescriptorSet(0).GetLayout()
		};

		const bool useLegacyDefaults = material.GetRuntimeLayout() == nullptr;
		ApplyMaterialRenderState(request, *materialPass, useLegacyDefaults);
		return true;
	}

	void EditorRenderer::ApplyMaterialRenderState(
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
			request.DepthCompareOp = VK_COMPARE_OP_LESS;
			request.EnableBlending = false;
			return;
		}

		request.CullMode = materialPass.RenderState.CullMode;
		request.EnableDepthTest = materialPass.RenderState.DepthTest;
		request.EnableDepthWrite = materialPass.RenderState.DepthWrite;
		request.DepthCompareOp = materialPass.RenderState.DepthCompareOp;
		request.EnableBlending = materialPass.RenderState.Blend;
	}

	VulkanGraphicsPipeline* EditorRenderer::GetPipeline(
		const RenderGraphTransientRenderTargetDesc& targetDesc,
		Ref<VulkanGeometry>& geometry,
		Ref<VulkanMaterial>& material,
		PassType passType,
		std::string* failureReason)
	{
		if (!m_PipelineFactory)
		{
			if (failureReason)
				*failureReason = "pipeline factory is null";
			return nullptr;
		}

		if (!geometry)
		{
			if (failureReason)
				*failureReason = "geometry is null";
			return nullptr;
		}

		if (!material)
		{
			if (failureReason)
				*failureReason = "material runtime is null";
			return nullptr;
		}

		if (!material->HasDescriptorSets())
		{
			if (failureReason)
				*failureReason = "material descriptor sets are not initialized";
			return nullptr;
		}

		PipelineRequest request{};
		if (!FillMaterialPipelineRequest(request, targetDesc, *geometry, *material, passType, failureReason))
		{
			return nullptr;
		}

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
		request.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		ApplyShaderLabRenderState(request, m_GridRenderState);
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
		request.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		ApplyShaderLabRenderState(request, m_TonemapRenderState);
		request.PushConstantStages = 0;
		request.PushConstantSize = 0;

		return m_PipelineFactory->GetOrCreate(request);
	}

	VulkanGraphicsPipeline* EditorRenderer::GetPickingPipeline(const VulkanRenderTargetView& rt, Ref<VulkanGeometry>& geometry)
	{
		if (!m_ViewportPickingPass || !m_PickingVertexShader || !m_PickingFragmentShader)
			return nullptr;

		PipelineRequest request{};
		request.Pass = PassType::EditorPicking;
		request.Geometry = geometry.get();
		request.VertexShader = m_PickingVertexShader.get();
		request.FragmentShader = m_PickingFragmentShader.get();

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
		request.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		ApplyShaderLabRenderState(request, m_PickingRenderState);
		request.PushConstantStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		request.PushConstantSize = ViewportPickingPushConstantSize;

		return m_PipelineFactory->GetOrCreate(request);
	}

VulkanGraphicsPipeline* EditorRenderer::GetSkyboxPipeline(const VulkanRenderTargetView& rt)
	{
		if (!m_SkyboxMaterial)
			return nullptr;

		const VulkanMaterial::PassRuntime* skyboxPass = m_SkyboxMaterial->FindPass(PassType::PostProcess);
		if (!skyboxPass || !skyboxPass->VertexShader || !skyboxPass->FragmentShader)
			return nullptr;

		PipelineRequest request{};
		request.Pass = PassType::PostProcess;
		request.UseVertexInput = false;
		request.VertexShader = skyboxPass->VertexShader.get();
		request.FragmentShader = skyboxPass->FragmentShader.get();

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
		request.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		ApplyShaderLabRenderState(request, skyboxPass->RenderState);
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
			m_DeferredLightingPass->SetIBLInput(m_IBL);
		}

		if (m_ForwardOpaquePass)
		{
			ScenePassData forwardSceneData = sceneData;
			forwardSceneData.BeginInfo.ClearColors = false;
			forwardSceneData.BeginInfo.ClearDepthAttachment = false;
			forwardSceneData.BeginInfo.TransitionSampledColors = true;
			forwardSceneData.BeginInfo.TransitionSampledDepth = false;
			m_ForwardOpaquePass->SetSceneData(forwardSceneData);
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
		if (m_ForwardOpaquePass)
			m_ForwardOpaquePass->ClearDrawItems();
		if (m_ViewportPickingPass)
			m_ViewportPickingPass->ClearDrawItems();

		uint32_t queuedGBufferCount = 0;
		uint32_t queuedForwardCount = 0;
		uint32_t runtimeMaterialCreateFailures = 0;
		uint32_t runtimeNoShaderCount = 0;
		uint32_t sceneCompatiblePassMissCount = 0;
		uint32_t pipelineFailureCount = 0;

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
				{
					++runtimeMaterialCreateFailures;
					LogMaterialDiagnosticOnce(
						materialHandle,
						object.GetName(),
						PassType::Unknown,
						"resource factory failed to create runtime material");
					continue;
				}

				if (!material->HasAnyShader())
				{
					++runtimeNoShaderCount;
					LogMaterialDiagnosticOnce(
						materialHandle,
						object.GetName(),
						PassType::Unknown,
						"runtime material has no valid shader stages",
						material.get());
					continue;
				}

				m_VulkanResFactory->RefreshMaterialFrameResources(
					materialHandle,
					m_Context->GetCurrentFrameIndex());

				RenderGraphTransientRenderTargetDesc gbufferDesc = m_GBufferTargetDesc;
				gbufferDesc.Width = surface.GetWidth();
				gbufferDesc.Height = surface.GetHeight();

				bool queuedScenePass = false;
				const VulkanMaterial::PassRuntime* gbufferPass = FindMaterialPassForScene(*material, PassType::GBuffer);
				if (gbufferPass)
				{
					std::string gbufferPipelineFailure;
					if (VulkanGraphicsPipeline* pipeline = GetPipeline(
						gbufferDesc,
						geometry,
						material,
						PassType::GBuffer,
						&gbufferPipelineFailure))
					{
						BasePassDrawItem item{};
						item.Pipeline = pipeline;
						item.Geometry = geometry.get();
						item.Material = material.get();
						item.PerObject = objectData;
						m_BasePass->AddDrawItem(item);
						++queuedGBufferCount;
						queuedScenePass = true;
						LogMaterialQueueSuccessOnce(
							materialHandle,
							object.GetName(),
							PassType::GBuffer,
							*pipeline,
							material.get());
					}
					else
					{
						++pipelineFailureCount;
						LogMaterialDiagnosticOnce(
							materialHandle,
							object.GetName(),
							PassType::GBuffer,
							gbufferPipelineFailure.empty() ? "pipeline creation returned null" : gbufferPipelineFailure,
							material.get());
					}
				}

				RenderGraphTransientRenderTargetDesc lightingDesc = m_LightingTargetDesc;
				lightingDesc.Width = surface.GetWidth();
				lightingDesc.Height = surface.GetHeight();
				const VulkanMaterial::PassRuntime* forwardOpaquePass =
					m_ForwardOpaquePass
					? FindMaterialPassForScene(*material, PassType::ForwardOpaque)
					: nullptr;
				if (forwardOpaquePass)
				{
					std::string forwardPipelineFailure;
					if (VulkanGraphicsPipeline* pipeline = GetPipeline(
						lightingDesc,
						geometry,
						material,
						PassType::ForwardOpaque,
						&forwardPipelineFailure))
					{
						ForwardOpaqueDrawItem item{};
						item.Pipeline = pipeline;
						item.Geometry = geometry.get();
						item.Material = material.get();
						item.PerObject = objectData;
						m_ForwardOpaquePass->AddDrawItem(item);
						++queuedForwardCount;
						queuedScenePass = true;
						LogMaterialQueueSuccessOnce(
							materialHandle,
							object.GetName(),
							PassType::ForwardOpaque,
							*pipeline,
							material.get());
					}
					else
					{
						++pipelineFailureCount;
						LogMaterialDiagnosticOnce(
							materialHandle,
							object.GetName(),
							PassType::ForwardOpaque,
							forwardPipelineFailure.empty() ? "pipeline creation returned null" : forwardPipelineFailure,
							material.get());
					}
				}

				if (!gbufferPass && !forwardOpaquePass)
				{
					++sceneCompatiblePassMissCount;
					LogMaterialDiagnosticOnce(
						materialHandle,
						object.GetName(),
						PassType::Unknown,
						"material has no scene-compatible pass (neither GBuffer nor ForwardOpaque)",
						material.get());
				}

				if (!queuedScenePass)
					continue;

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

		if (!m_LoggedSceneSubmissionSummary)
		{
			m_LoggedSceneSubmissionSummary = true;
			KITA_CORE_WARN(
				"EditorRenderer scene submission summary: gbufferQueued={}, forwardQueued={}, materialCreateFailures={}, noShaderMaterials={}, noScenePassMaterials={}, pipelineFailures={}",
				queuedGBufferCount,
				queuedForwardCount,
				runtimeMaterialCreateFailures,
				runtimeNoShaderCount,
				sceneCompatiblePassMissCount,
				pipelineFailureCount);
		}


		BuildRenderGraph(surface,finalRt, pickingRt);
		m_RenderGraph->Execute(*m_Context, cmd);
		UpdateRenderGraphPreviewTextures(finalRt);
	}

}
