#include "kita_pch.h"
#include "VulkanResourceFactory.h"
#include "asset/AssetManager.h"
#include "render/VulkanContext.h"
#include "render/VulkanTextureLoader.h"
#include "render/VulkanGeometry.h"
#include "core/Log.h"

namespace Kita {

	namespace
	{
		float BoolToGpuFlag(bool value)
		{
			return value ? 1.0f : 0.0f;
		}

		BufferLayout CreateMeshVertexLayout()
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
	}

	VulkanResourceFactory::VulkanResourceFactory(VulkanContext& context, AssetManager& assetManager)
		:m_Context(context), m_AssetManager(assetManager)
	{

	}

	void VulkanResourceFactory::SetFallbackTextureHandles(
		AssetHandle whiteTextureHandle,
		AssetHandle blackTextureHandle,
		AssetHandle normalTextureHandle)
	{
		m_FallbackWhiteTextureHandle = whiteTextureHandle;
		m_FallbackBlackTextureHandle = blackTextureHandle;
		m_FallbackNormalTextureHandle = normalTextureHandle;
	}

	VulkanResourceFactory::ShaderBundle VulkanResourceFactory::GetOrCreateShaderBundle(AssetHandle handle)
	{
		if (!Asset::IsValidHandle(handle))
		{
			return {};
		}

		auto it = m_ShaderCache.find(handle);
		if (it != m_ShaderCache.end())
		{
			return it->second;
		}

		Ref<ShaderAsset> shaderAsset = m_AssetManager.GetShaderAsset(handle);
		if (!shaderAsset)
		{
			KITA_CORE_WARN("VulkanResourceFactory: shader asset not found, handle={}", handle);
			return {};
		}

		ShaderBundle bundle{};

		if (shaderAsset->VertexStage.Valid())
		{
			bundle.VertexShader = BuildShader(
				*shaderAsset,
				shaderAsset->VertexStage,
				VK_SHADER_STAGE_VERTEX_BIT,
				"VS");
		}

		if (shaderAsset->FragmentStage.Valid())
		{
			bundle.FragmentShader = BuildShader(
				*shaderAsset,
				shaderAsset->FragmentStage,
				VK_SHADER_STAGE_FRAGMENT_BIT,
				"FS");
		}

		if (!bundle.IsValid())
		{
			KITA_CORE_WARN("VulkanResourceFactory: shader bundle is incomplete, handle={}", handle);
			return {};
		}

		m_ShaderCache[handle] = bundle;
		return bundle;
	}

	Ref<VulkanTexture> VulkanResourceFactory::GetOrCreateTexture(AssetHandle handle)
	{
		if (!Asset::IsValidHandle(handle))
		{
			return {};
		}

		auto it = m_TextureCache.find(handle);
		if (it != m_TextureCache.end())
		{
			return it->second;
		}

		Ref<TextureAsset> textureAsset = m_AssetManager.GetTextureAsset(handle);
		if (!textureAsset)
		{
			KITA_CORE_WARN("VulkanResourceFactory: texture asset not found, handle={}", handle);
			return {};
		}

		Ref<VulkanTexture> vulkanTexture = VulkanTextureLoader::LoadTexture(m_Context, *textureAsset);
		if (!vulkanTexture)
		{
			KITA_CORE_WARN("Failed to create runtime VulkanTexture for asset: {}", textureAsset->SourcePath.string());
			return {};
		}

		m_TextureCache[handle] = vulkanTexture;
		return vulkanTexture;
	}

	Ref<VulkanMaterial> VulkanResourceFactory::CreateMaterial(AssetHandle handle)
	{
		if (!Asset::IsValidHandle(handle))
		{
			return {};
		}

		auto cacheIt = m_MaterialCache.find(handle);
		if (cacheIt != m_MaterialCache.end())
		{
			return cacheIt->second;
		}

		Ref<MaterialAsset> materialAsset = m_AssetManager.GetMaterialAsset(handle);
		if (!materialAsset)
		{
			KITA_CORE_WARN("VulkanResourceFactory: material asset not found, handle={}", handle);
			return {};
		}

		Ref<VulkanMaterial> material = CreateMaterial(*materialAsset);
		if (material)
			m_MaterialCache[handle] = material;
		return material;
	}

	Ref<VulkanMaterial> VulkanResourceFactory::CreateMaterial(const MaterialAsset& materialAsset)
	{
		Ref<VulkanMaterial> material = CreateRef<VulkanMaterial>();
		ApplyMaterial(materialAsset, *material);
		return material;
	}

	void VulkanResourceFactory::ApplyMaterial(const MaterialAsset& materialAsset, VulkanMaterial& outMaterial)
	{
		const Ref<VulkanTexture> fallbackWhiteTexture =
			Asset::IsValidHandle(m_FallbackWhiteTextureHandle)
			? GetOrCreateTexture(m_FallbackWhiteTextureHandle)
			: nullptr;
		const Ref<VulkanTexture> fallbackBlackTexture =
			Asset::IsValidHandle(m_FallbackBlackTextureHandle)
			? GetOrCreateTexture(m_FallbackBlackTextureHandle)
			: nullptr;
		const Ref<VulkanTexture> fallbackNormalTexture =
			Asset::IsValidHandle(m_FallbackNormalTextureHandle)
			? GetOrCreateTexture(m_FallbackNormalTextureHandle)
			: nullptr;

		outMaterial.SetFallbackTextures(
			fallbackWhiteTexture,
			fallbackBlackTexture,
			fallbackNormalTexture);

		MaterialGpuParams params{};
		params.BaseColor = materialAsset.m_SurfaceParams.BaseColor;
		params.Emissive = glm::vec4(materialAsset.m_SurfaceParams.Emissive, 0.0f);
		params.SurfaceParams = glm::vec4(
			materialAsset.m_SurfaceParams.Metallic,
			materialAsset.m_SurfaceParams.Roughness,
			materialAsset.m_SurfaceParams.AmbientOcclusion,
			materialAsset.m_SurfaceParams.Opacity);
		params.MiscParams = glm::vec4(
			materialAsset.m_SurfaceParams.NormalScale,
			materialAsset.m_SurfaceParams.AlphaCutoff,
			0.0f,
			0.0f);
		outMaterial.SetParams(params);

		outMaterial.SetVertexShader(nullptr);
		outMaterial.SetFragmentShader(nullptr);
		outMaterial.ClearTextures();

		if (Asset::IsValidHandle(materialAsset.ShaderHandle))
		{
			ShaderBundle shaderBundle = GetOrCreateShaderBundle(materialAsset.ShaderHandle);
			if (shaderBundle.IsValid())
			{
				outMaterial.SetVertexShader(shaderBundle.VertexShader);
				outMaterial.SetFragmentShader(shaderBundle.FragmentShader);
			}
			else
			{
				KITA_CORE_WARN(
					"VulkanResourceFactory: failed to build shader bundle for material, shader handle={}",
					materialAsset.ShaderHandle);
			}
		}

		if (Asset::IsValidHandle(materialAsset.m_Textures.Albedo))
		{
			Ref<VulkanTexture> texture = GetOrCreateTexture(materialAsset.m_Textures.Albedo);
			if (texture)
			{
				outMaterial.SetAlbedoTexture(texture);
			}
			else
			{
				KITA_CORE_WARN(
					"VulkanResourceFactory: failed to build texture for material, texture handle={}",
					materialAsset.m_Textures.Albedo);
			}
		}

		if (Asset::IsValidHandle(materialAsset.m_Textures.Normal))
		{
			outMaterial.SetNormalTexture(GetOrCreateTexture(materialAsset.m_Textures.Normal));
		}
		if (Asset::IsValidHandle(materialAsset.m_Textures.MetallicRoughness))
		{
			outMaterial.SetMetallicRoughnessTexture(GetOrCreateTexture(materialAsset.m_Textures.MetallicRoughness));
		}
		if (Asset::IsValidHandle(materialAsset.m_Textures.AmbientOcclusion))
		{
			outMaterial.SetAmbientOcclusionTexture(GetOrCreateTexture(materialAsset.m_Textures.AmbientOcclusion));
		}
		if (Asset::IsValidHandle(materialAsset.m_Textures.Emissive))
		{
			outMaterial.SetEmissiveTexture(GetOrCreateTexture(materialAsset.m_Textures.Emissive));
		}
		if (Asset::IsValidHandle(materialAsset.m_Textures.Opacity))
		{
			outMaterial.SetOpacityTexture(GetOrCreateTexture(materialAsset.m_Textures.Opacity));
		}

		if (!outMaterial.GetAlbedoTexture() && fallbackWhiteTexture)
		{
			outMaterial.SetAlbedoTexture(fallbackWhiteTexture);
		}

		if (!outMaterial.GetNormalTexture() && fallbackNormalTexture)
		{
			outMaterial.SetNormalTexture(fallbackNormalTexture);
		}

		if (!outMaterial.GetMetallicRoughnessTexture() && fallbackWhiteTexture)
		{
			outMaterial.SetMetallicRoughnessTexture(fallbackWhiteTexture);
		}

		if (!outMaterial.GetAmbientOcclusionTexture() && fallbackWhiteTexture)
		{
			outMaterial.SetAmbientOcclusionTexture(fallbackWhiteTexture);
		}

		if (!outMaterial.GetEmissiveTexture() && fallbackBlackTexture)
		{
			outMaterial.SetEmissiveTexture(fallbackBlackTexture);
		}

		if (!outMaterial.GetOpacityTexture() && fallbackWhiteTexture)
		{
			outMaterial.SetOpacityTexture(fallbackWhiteTexture);
		}

		params.TextureFlags0 = glm::vec4(
			BoolToGpuFlag(Asset::IsValidHandle(materialAsset.m_Textures.Albedo)),
			BoolToGpuFlag(Asset::IsValidHandle(materialAsset.m_Textures.Normal)),
			BoolToGpuFlag(Asset::IsValidHandle(materialAsset.m_Textures.MetallicRoughness)),
			BoolToGpuFlag(Asset::IsValidHandle(materialAsset.m_Textures.AmbientOcclusion)));
		params.TextureFlags1 = glm::vec4(
			BoolToGpuFlag(Asset::IsValidHandle(materialAsset.m_Textures.Emissive)),
			BoolToGpuFlag(Asset::IsValidHandle(materialAsset.m_Textures.Opacity)),
			0.0f,
			0.0f);
		outMaterial.SetParams(params);

		if (outMaterial.HasAnyTexture() || outMaterial.GetVertexShader() || outMaterial.GetFragmentShader())
		{
			outMaterial.EnsureDescriptors(m_Context, m_Context.GetFramesInFlight());
			outMaterial.MarkDescriptorSetsDirty();
		}
	}

	void VulkanResourceFactory::RefreshMaterial(AssetHandle handle)
	{
		if (!Asset::IsValidHandle(handle))
		{
			return;
		}

		auto cacheIt = m_MaterialCache.find(handle);
		if (cacheIt == m_MaterialCache.end() || !cacheIt->second)
		{
			return;
		}

		Ref<MaterialAsset> materialAsset = m_AssetManager.GetMaterialAsset(handle);
		if (!materialAsset)
		{
			KITA_CORE_WARN("VulkanResourceFactory: material asset not found during refresh, handle={}", handle);
			return;
		}

		ApplyMaterial(*materialAsset, *cacheIt->second);
	}

	void VulkanResourceFactory::RefreshMaterialFrameResources(AssetHandle handle, uint32_t frameIndex)
	{
		if (!Asset::IsValidHandle(handle))
		{
			return;
		}

		auto cacheIt = m_MaterialCache.find(handle);
		if (cacheIt == m_MaterialCache.end() || !cacheIt->second)
		{
			return;
		}

		Ref<VulkanMaterial>& material = cacheIt->second;
		material->EnsureDescriptors(m_Context, m_Context.GetFramesInFlight());
		if (material->IsDescriptorSetDirty(frameIndex))
		{
			material->UpdateDescriptorSet(frameIndex);
		}
	}

	std::vector<Ref<VulkanGeometry>> VulkanResourceFactory::GetOrCreateGeometries(AssetHandle handle)
	{
		if (!Asset::IsValidHandle(handle))
		{
			return {};
		}

		auto it = m_GeometryCache.find(handle);
		if (it != m_GeometryCache.end())
		{
			return it->second;
		}

		Ref<MeshAsset> meshAsset = m_AssetManager.GetMeshAsset(handle);
		if (!meshAsset)
		{
			KITA_CORE_WARN("VulkanResourceFactory: mesh asset not found, handle={}", handle);
			return {};
		}

		return GetOrCreateGeometries(*meshAsset);
	}

	std::vector<Ref<VulkanGeometry>> VulkanResourceFactory::GetOrCreateGeometries(const MeshAsset& meshAsset)
	{
		if (!Asset::IsValidHandle(meshAsset.m_Handle))
		{
			return {};
		}

		auto it = m_GeometryCache.find(meshAsset.m_Handle);
		if (it != m_GeometryCache.end())
		{
			return it->second;
		}

		BufferLayout meshLayout = CreateMeshVertexLayout();
		std::vector<Ref<VulkanGeometry>> geometries{};
		for (const auto& rawData : meshAsset.MeshRawData)
		{
			if (rawData.Vertices.empty())
				continue;

			VulkanGeometry::CreateInfo createInfo{};
			createInfo.Name = "MeshGeometry";
			createInfo.VertexData = rawData.Vertices.data();
			createInfo.VertexDataSize = static_cast<uint32_t>(sizeof(Vertex) * rawData.Vertices.size());
			createInfo.VertexCount = static_cast<uint32_t>(rawData.Vertices.size());
			createInfo.VertexLayout = meshLayout;
			createInfo.IndexData = rawData.Indices.empty() ? nullptr : rawData.Indices.data();
			createInfo.IndexCount = static_cast<uint32_t>(rawData.Indices.size());
			createInfo.Dynamic = false;

			Ref<VulkanGeometry> geometry = CreateRef<VulkanGeometry>(m_Context, createInfo);
			geometries.push_back(geometry);
		}

		m_GeometryCache[meshAsset.m_Handle] = geometries;
		return geometries;
	}

	void VulkanResourceFactory::InvalidateShader(AssetHandle shaderHandle)
	{
		m_ShaderCache.erase(shaderHandle);
	}

	void VulkanResourceFactory::InvalidateTexture(AssetHandle textureHandle)
	{
		m_TextureCache.erase(textureHandle);
	}

	void VulkanResourceFactory::InvalidateMaterial(AssetHandle materialHandle)
	{
		m_MaterialCache.erase(materialHandle);
	}

	void VulkanResourceFactory::InvalidateMesh(AssetHandle meshHandle)
	{
		m_GeometryCache.erase(meshHandle);
	}

	void VulkanResourceFactory::Clear()
	{
		m_ShaderCache.clear();
		m_TextureCache.clear();
		m_MaterialCache.clear();
		m_GeometryCache.clear();
	}

	Ref<VulkanShader> VulkanResourceFactory::BuildShader(const ShaderAsset& shaderAsset, const ShaderStageBinary& stageBinary, VkShaderStageFlagBits stage, const char* suffix)
	{
		if (!stageBinary.Valid())
		{
			return nullptr;
		}

		VulkanShader::CreateInfo createInfo{};
		const std::string baseName =
			shaderAsset.SourcePath.stem().string().empty()
			? std::to_string(shaderAsset.m_Handle)
			: shaderAsset.SourcePath.stem().string();

		createInfo.Name = baseName + "_" + suffix;
		createInfo.Stage = stage;
		createInfo.EntryPoint = stageBinary.EntryPoint.empty() ? "main" : stageBinary.EntryPoint;
		createInfo.Spirv = stageBinary.Spirv;

		try
		{
			return CreateRef<VulkanShader>(&m_Context, createInfo);
		}
		catch (const std::exception& e)
		{
			KITA_CORE_ERROR(
				"VulkanResourceFactory: failed to create shader '{}', reason: {}",
				createInfo.Name,
				e.what());
			return nullptr;
		}
	}

}
