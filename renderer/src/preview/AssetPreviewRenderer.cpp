#include "renderer_pch.h"
#include "AssetPreviewRenderer.h"

#include "PreviewSpherePass.h"
#include "asset/AssetManager.h"
#include "core/Log.h"
#include "file/Project.h"
#include "render/BufferLayout.h"
#include "render/VulkanContext.h"
#include "render/VulkanImage.h"
#include "render/VulkanGeometry.h"
#include "render/VulkanGraphicsPipeline.h"
#include "render/VulkanMaterial.h"
#include "render/VulkanRenderTarget.h"
#include "render/VulkanResourceFactory.h"
#include "render/VulkanTexture.h"
#include "render/mesh/Mesh.h"
#include "render/pass/RenderContext.h"
#include "render/pass/SceneBindings.h"
#include "render/pipeline/PipelineFactory.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

namespace Kita {

	namespace
	{
		constexpr const char* kCubemapPreviewShaderPath = "packages/render/shaders/CubemapPreview.slang";
		constexpr uint32_t kSphereSegments = 48;
		constexpr uint32_t kSphereRings = 24;

		uint32_t NormalizePreviewSize(uint32_t size)
		{
			const uint32_t clamped = std::clamp(size, 32u, 512u);
			return clamped;
		}

		VulkanRenderTarget::CreateInfo BuildPreviewTargetCreateInfo(uint32_t size)
		{
			VulkanRenderTarget::CreateInfo createInfo{};
			createInfo.Name = "AssetPreviewTarget_" + std::to_string(size);
			createInfo.Width = size;
			createInfo.Height = size;
			createInfo.Samples = VK_SAMPLE_COUNT_1_BIT;

			VulkanRenderTarget::ColorAttachmentDesc color{};
			color.Name = createInfo.Name + "_Color";
			color.Format = VK_FORMAT_R8G8B8A8_UNORM;
			color.CreateSampler = true;
			color.Filter = VK_FILTER_LINEAR;
			color.AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			color.ExtraUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			createInfo.ColorAttachments.push_back(color);

			VulkanRenderTarget::DepthAttachmentDesc depth{};
			depth.Enabled = true;
			depth.Name = createInfo.Name + "_Depth";
			depth.Format = VK_FORMAT_D32_SFLOAT;
			createInfo.DepthAttachment = depth;
			return createInfo;
		}

		BufferLayout CreatePreviewVertexLayout()
		{
			return BufferLayout{
				{ ShaderDataType::Float3, "position" },
				{ ShaderDataType::Float4, "color" },
				{ ShaderDataType::Float2, "texcoords" },
				{ ShaderDataType::Float3, "normal" },
				{ ShaderDataType::Float3, "tangent" },
				{ ShaderDataType::Float3, "bitangent" }
			};
		}

		void BuildPreviewSphere(std::vector<Vertex>& outVertices, std::vector<uint32_t>& outIndices)
		{
			outVertices.clear();
			outIndices.clear();

			for (uint32_t ring = 0; ring <= kSphereRings; ++ring)
			{
				const float v = static_cast<float>(ring) / static_cast<float>(kSphereRings);
				const float theta = v * glm::pi<float>();
				const float sinTheta = std::sin(theta);
				const float cosTheta = std::cos(theta);

				for (uint32_t segment = 0; segment <= kSphereSegments; ++segment)
				{
					const float u = static_cast<float>(segment) / static_cast<float>(kSphereSegments);
					const float phi = u * glm::two_pi<float>();
					const float sinPhi = std::sin(phi);
					const float cosPhi = std::cos(phi);

					const glm::vec3 normal(
						sinTheta * cosPhi,
						cosTheta,
						sinTheta * sinPhi);
					const glm::vec3 tangent = glm::normalize(glm::vec3(-sinPhi, 0.0f, cosPhi));
					const glm::vec3 bitangent = glm::normalize(glm::cross(normal, tangent));

					Vertex vertex{};
					vertex.position = normal;
					vertex.color = glm::vec4(1.0f);
					vertex.texcoords = glm::vec2(u, v);
					vertex.normal = normal;
					vertex.tangent = tangent;
					vertex.bitangent = bitangent;
					outVertices.push_back(vertex);
				}
			}

			const uint32_t stride = kSphereSegments + 1;
			for (uint32_t ring = 0; ring < kSphereRings; ++ring)
			{
				for (uint32_t segment = 0; segment < kSphereSegments; ++segment)
				{
					const uint32_t i0 = ring * stride + segment;
					const uint32_t i1 = i0 + 1;
					const uint32_t i2 = i0 + stride;
					const uint32_t i3 = i2 + 1;

					outIndices.push_back(i0);
					outIndices.push_back(i2);
					outIndices.push_back(i1);
					outIndices.push_back(i1);
					outIndices.push_back(i2);
					outIndices.push_back(i3);
				}
			}
		}

		ObjectData CreateIdentityObjectData()
		{
			ObjectData objectData{};
			const glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(0.72f));
			objectData.Matrix_M = model;
			objectData.Matrix_I_M = glm::inverse(model);
			return objectData;
		}
	}

	AssetPreviewRenderer::AssetPreviewRenderer(VulkanContext& context, VulkanResourceFactory& resourceFactory)
		: m_Context(&context)
		, m_ResourceFactory(&resourceFactory)
	{
	}

	AssetPreviewRenderer::~AssetPreviewRenderer()
	{
		Clear();
	}

	PreviewThumbnailHandle AssetPreviewRenderer::GetOrRender(const AssetPreviewRequest& request)
	{
		if (!Asset::IsValidHandle(request.Handle))
		{
			return {};
		}

		AssetPreviewRequest normalizedRequest = request;
		normalizedRequest.Size = NormalizePreviewSize(request.Size);

		const PreviewThumbnailKey key = MakeKey(normalizedRequest);
		if (PreviewThumbnailHandle cached = m_ThumbnailCache.Find(key); cached.IsValid())
		{
			return cached;
		}

		const VulkanImage* image = RenderPreviewImage(normalizedRequest);
		if (!image)
		{
			return {};
		}

		return m_ThumbnailCache.StoreRenderedImage(key, *image, normalizedRequest.Size, normalizedRequest.Size);
	}

	void AssetPreviewRenderer::Invalidate(AssetHandle handle)
	{
		if (m_Context)
		{
			m_Context->WaitIdle();
		}
		m_ThumbnailCache.Invalidate(handle);
		m_CubemapPreviewMaterials.erase(handle);
	}

	void AssetPreviewRenderer::Clear()
	{
		m_ThumbnailCache.Clear();
		m_Targets.clear();
		m_CubemapPreviewMaterials.clear();
		m_SphereGeometry = nullptr;
		m_PipelineFactory.reset();
		m_SceneBindings.reset();
	}

	PreviewThumbnailKey AssetPreviewRenderer::MakeKey(const AssetPreviewRequest& request) const
	{
		PreviewThumbnailKey key{};
		key.Handle = request.Handle;
		key.Type = request.Type;
		key.Size = NormalizePreviewSize(request.Size);
		key.Revision = request.Revision;
		return key;
	}

	const VulkanImage* AssetPreviewRenderer::RenderPreviewImage(const AssetPreviewRequest& request)
	{
		switch (request.Type)
		{
		case AssetPreviewType::CubemapSphere:
			return RenderCubemapSpherePreview(request.Handle, request.Size);
		case AssetPreviewType::MaterialSphere:
			return RenderMaterialSpherePreview(request.Handle, request.Size);
		case AssetPreviewType::Texture2D:
		default:
			return nullptr;
		}
	}

	const VulkanImage* AssetPreviewRenderer::RenderCubemapSpherePreview(AssetHandle handle, uint32_t size)
	{
		if (!m_Context || !m_ResourceFactory)
		{
			return nullptr;
		}

		Ref<VulkanTexture> texture = m_ResourceFactory->GetOrCreateTexture(handle);
		if (!texture || !texture->IsValid() || texture->GetType() != TextureType::TextureCube)
		{
			return nullptr;
		}

		PreviewTarget& target = GetOrCreateTarget(size);
		if (!target.RenderTarget || !target.RenderTarget->IsValid())
		{
			return nullptr;
		}

		if (!EnsureSharedResources())
		{
			return nullptr;
		}

		Ref<VulkanMaterial> previewMaterial = GetOrCreateCubemapPreviewMaterial(handle, texture);
		if (!previewMaterial)
		{
			return nullptr;
		}

		const uint32_t frameIndex = m_Context->GetCurrentFrameIndex();
		previewMaterial->EnsureDescriptors(*m_Context, m_Context->GetFramesInFlight());
		if (previewMaterial->IsDescriptorSetDirty(frameIndex))
		{
			previewMaterial->UpdateDescriptorSet(frameIndex);
		}

		VulkanGraphicsPipeline* pipeline = GetCubemapPreviewPipeline(target, *previewMaterial);
		if (!pipeline || !target.SpherePass)
		{
			return nullptr;
		}

		PreviewSphereDrawItem drawItem{};
		drawItem.Pipeline = pipeline;
		drawItem.Geometry = m_SphereGeometry.get();
		drawItem.Material = previewMaterial.get();
		drawItem.PerObject = CreateIdentityObjectData();
		target.SpherePass->SetDrawItem(drawItem);
		target.SpherePass->SetSceneData(BuildPreviewSceneData());

		VulkanRenderTargetView targetView = target.RenderTarget->CreateView();
		RenderPassContext passContext(*m_Context, m_Context->GetCurrentCommandBuffer(), targetView);
		target.SpherePass->Execute(passContext);
		return &target.RenderTarget->GetSampledColorAttachment(0);
	}

	const VulkanImage* AssetPreviewRenderer::RenderMaterialSpherePreview(AssetHandle handle, uint32_t size)
	{
		if (!m_Context || !m_ResourceFactory)
		{
			return nullptr;
		}

		if (!Asset::IsValidHandle(handle))
		{
			return nullptr;
		}

		PreviewTarget& target = GetOrCreateTarget(size);
		if (!target.RenderTarget || !target.RenderTarget->IsValid())
		{
			return nullptr;
		}

		// TODO: 与 cubemap 预览共用离屏 target、球体 mesh、相机和灯光，只替换为真实材质绑定。
		return &target.RenderTarget->GetSampledColorAttachment(0);
	}

	AssetPreviewRenderer::PreviewTarget& AssetPreviewRenderer::GetOrCreateTarget(uint32_t size)
	{
		const uint32_t normalizedSize = NormalizePreviewSize(size);
		auto it = m_Targets.find(normalizedSize);
		if (it != m_Targets.end())
		{
			return it->second;
		}

		PreviewTarget target{};
		target.Size = normalizedSize;
		target.RenderTarget = CreateUnique<VulkanRenderTarget>(*m_Context, BuildPreviewTargetCreateInfo(normalizedSize));
		if (m_SceneBindings)
		{
			target.SpherePass = CreateUnique<PreviewSpherePass>(*m_SceneBindings, MakePreviewSpherePassDesc(target.RenderTarget->CreateView()));
		}

		auto [insertedIt, inserted] = m_Targets.emplace(normalizedSize, std::move(target));
		(void)inserted;
		return insertedIt->second;
	}

	bool AssetPreviewRenderer::EnsureSharedResources()
	{
		if (!m_Context || !m_ResourceFactory)
		{
			return false;
		}

		if (!m_SceneBindings)
		{
			m_SceneBindings = CreateUnique<SceneBindings>();
			m_SceneBindings->Init(*m_Context, m_Context->GetFramesInFlight());

			for (auto& [size, target] : m_Targets)
			{
				(void)size;
				if (target.RenderTarget && !target.SpherePass)
				{
					target.SpherePass = CreateUnique<PreviewSpherePass>(*m_SceneBindings, MakePreviewSpherePassDesc(target.RenderTarget->CreateView()));
				}
			}
		}

		if (!m_PipelineFactory)
		{
			m_PipelineFactory = CreateUnique<PipelineFactory>(*m_Context);
		}

		if (!m_SphereGeometry)
		{
			std::vector<Vertex> vertices;
			std::vector<uint32_t> indices;
			BuildPreviewSphere(vertices, indices);

			VulkanGeometry::CreateInfo createInfo{};
			createInfo.Name = "PreviewSphereGeometry";
			createInfo.VertexData = vertices.data();
			createInfo.VertexDataSize = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));
			createInfo.VertexCount = static_cast<uint32_t>(vertices.size());
			createInfo.VertexLayout = CreatePreviewVertexLayout();
			createInfo.IndexData = indices.data();
			createInfo.IndexCount = static_cast<uint32_t>(indices.size());
			createInfo.Dynamic = false;

			m_SphereGeometry = CreateRef<VulkanGeometry>(*m_Context, createInfo);
		}

		return m_SphereGeometry != nullptr;
	}

	Ref<VulkanMaterial> AssetPreviewRenderer::GetOrCreateCubemapPreviewMaterial(AssetHandle handle, const Ref<VulkanTexture>& texture)
	{
		if (!Asset::IsValidHandle(handle))
		{
			return nullptr;
		}

		if (!texture || !texture->IsValid() || texture->GetType() != TextureType::TextureCube)
		{
			return nullptr;
		}

		auto materialIt = m_CubemapPreviewMaterials.find(handle);
		if (materialIt != m_CubemapPreviewMaterials.end() && materialIt->second)
		{
			return materialIt->second;
		}

		AssetManager& assetManager = AssetManager::GetInstance();
		if (!Asset::IsValidHandle(m_CubemapPreviewShaderHandle))
		{
			m_CubemapPreviewShaderHandle = assetManager.GetHandleByPath(kCubemapPreviewShaderPath);
			if (!Asset::IsValidHandle(m_CubemapPreviewShaderHandle))
			{
				if (const Ref<Project> project = Project::GetActive())
				{
					m_CubemapPreviewShaderHandle =
						assetManager.ImportAsset(project->GetAssetRootDirectory() / kCubemapPreviewShaderPath);
				}
			}
		}

		if (!Asset::IsValidHandle(m_CubemapPreviewShaderHandle))
		{
			KITA_CORE_WARN("AssetPreviewRenderer: missing cubemap preview shader '{}'", kCubemapPreviewShaderPath);
			return nullptr;
		}

		VulkanResourceFactory::ShaderBundle shaderBundle = m_ResourceFactory->GetOrCreateShaderBundle(m_CubemapPreviewShaderHandle);
		if (!shaderBundle.IsValid())
		{
			return nullptr;
		}

		Ref<VulkanMaterial> material = CreateRef<VulkanMaterial>();
		MaterialGpuParams params{};
		params.BaseColor = glm::vec4(1.0f);
		params.SurfaceParams = glm::vec4(0.0f, 0.18f, 1.0f, 1.0f);
		material->SetParams(params);
		material->SetVertexShader(shaderBundle.VertexShader);
		material->SetFragmentShader(shaderBundle.FragmentShader);
		material->SetAlbedoTexture(texture);
		material->EnsureDescriptors(*m_Context, m_Context->GetFramesInFlight());
		m_CubemapPreviewMaterials[handle] = material;
		return material;
	}

	VulkanGraphicsPipeline* AssetPreviewRenderer::GetCubemapPreviewPipeline(PreviewTarget& target, VulkanMaterial& material)
	{
		if (!m_PipelineFactory || !m_SphereGeometry || !target.RenderTarget)
		{
			return nullptr;
		}

		PipelineRequest request{};
		request.Pass = PassType::ForwardOpaque;
		request.Geometry = m_SphereGeometry.get();
		request.UseVertexInput = true;
		request.VertexShader = material.GetVertexShader().get();
		request.FragmentShader = material.GetFragmentShader().get();
		request.ColorFormats.push_back(target.RenderTarget->GetColorFormat(0));
		request.DepthFormat = target.RenderTarget->HasDepthAttachment() ? target.RenderTarget->GetDepthFormat() : VK_FORMAT_UNDEFINED;
		request.Samples = target.RenderTarget->CreateView().GetSamples();
		request.DescriptorSetLayouts = {
			m_SceneBindings->GetDescriptorSet(m_Context->GetCurrentFrameIndex()).GetLayout(),
			material.GetDescriptorSet(m_Context->GetCurrentFrameIndex()).GetLayout()
		};
		request.CullMode = VK_CULL_MODE_BACK_BIT;
		request.FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		request.EnableDepthTest = true;
		request.EnableDepthWrite = true;
		request.DepthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
		request.EnableBlending = false;
		request.PushConstantStages = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		request.PushConstantSize = ObjectDataSize;

		return m_PipelineFactory->GetOrCreate(request);
	}

	ScenePassData AssetPreviewRenderer::BuildPreviewSceneData()
	{
		const glm::vec3 eye(0.0f, 0.0f, 3.0f);
		const glm::vec3 center(0.0f, 0.0f, 0.0f);
		const glm::vec3 up(0.0f, 1.0f, 0.0f);

		ScenePassData sceneData{};
		sceneData.Camera.Matrix_V = glm::lookAtRH(eye, center, up);
		sceneData.Camera.Matrix_P = glm::perspectiveRH_ZO(glm::radians(45.0f), 1.0f, 0.1f, 32.0f);
		sceneData.Camera.Matrix_VP = sceneData.Camera.Matrix_P * sceneData.Camera.Matrix_V;
		sceneData.Camera.Matrix_I_V = glm::inverse(sceneData.Camera.Matrix_V);
		sceneData.Camera.Matrix_I_P = glm::inverse(sceneData.Camera.Matrix_P);
		sceneData.Camera.Matrix_I_VP = glm::inverse(sceneData.Camera.Matrix_VP);
		sceneData.Camera.CameraPosWS = glm::vec4(eye, 1.0f);

		sceneData.MainLight.Direction = glm::vec4(glm::normalize(glm::vec3(-0.35f, -0.55f, -0.75f)), 0.0f);
		sceneData.MainLight.Color = glm::vec4(1.0f, 0.96f, 0.9f, 1.0f);
		sceneData.BeginInfo.ClearColor = glm::vec4(0.12f, 0.12f, 0.125f, 1.0f);
		sceneData.BeginInfo.ClearDepth = 1.0f;
		sceneData.BeginInfo.ClearStencil = 0;
		sceneData.BeginInfo.ClearColors = true;
		sceneData.BeginInfo.ClearDepthAttachment = true;
		sceneData.BeginInfo.TransitionSampledColors = true;
		sceneData.BeginInfo.TransitionSampledDepth = false;
		return sceneData;
	}

}
