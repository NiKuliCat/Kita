#include "kita_pch.h"
#include "VulkanResourceFactory.h"
#include "VulkanMaterial.h"
#include "asset/AssetManager.h"
#include "render/VulkanContext.h"
#include "render/VulkanTextureLoader.h"
#include "render/VulkanGeometry.h"
#include "core/Log.h"

namespace Kita {

	namespace
	{
		struct NormalizedShaderLabPropertyBlock
		{
			MaterialPropertyBlock PropertyBlock{};
			uint32_t FilledDefaults = 0;
			uint32_t MovedToOrphans = 0;
		};

		void MirrorShaderLabTextureToLegacySlot(
			VulkanMaterial& material,
			const std::string& propertyName,
			const Ref<VulkanTexture>& texture)
		{
			if (!texture)
				return;

			// 兼容仍然通过 legacy 纹理访问器取图的运行时路径，例如 skybox 纹理同步。
			if (propertyName == "_Albedo")
			{
				material.SetAlbedoTexture(texture);
				return;
			}

			if (propertyName == "_Normal")
			{
				material.SetNormalTexture(texture);
				return;
			}

			if (propertyName == "_MetallicRoughness")
			{
				material.SetMetallicRoughnessTexture(texture);
				return;
			}

			if (propertyName == "_AmbientOcclusionTexture")
			{
				material.SetAmbientOcclusionTexture(texture);
				return;
			}

			if (propertyName == "_EmissionMask" || propertyName == "_EmissiveTexture")
			{
				material.SetEmissiveTexture(texture);
				return;
			}

			if (propertyName == "_OpacityTexture")
			{
				material.SetOpacityTexture(texture);
			}
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

		// Packs legacy inspector data into the old GPU parameter layout.
		MaterialGpuParams BuildLegacyGpuParams(const MaterialAsset& materialAsset)
		{
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
			params.TextureFlags0 = glm::vec4(
				Asset::IsValidHandle(materialAsset.m_Textures.Albedo) ? 1.0f : 0.0f,
				Asset::IsValidHandle(materialAsset.m_Textures.Normal) ? 1.0f : 0.0f,
				Asset::IsValidHandle(materialAsset.m_Textures.MetallicRoughness) ? 1.0f : 0.0f,
				Asset::IsValidHandle(materialAsset.m_Textures.AmbientOcclusion) ? 1.0f : 0.0f);
			params.TextureFlags1 = glm::vec4(
				Asset::IsValidHandle(materialAsset.m_Textures.Emissive) ? 1.0f : 0.0f,
				Asset::IsValidHandle(materialAsset.m_Textures.Opacity) ? 1.0f : 0.0f,
				0.0f,
				0.0f);
			return params;
		}

		// 运行时也做一遍属性归一化，避免未经过材质编辑器保存的 ShaderLab 材质
		// 因缺少默认值而把 UBO 写成 0，最终在 GBuffer 阶段被整批 discard。
		NormalizedShaderLabPropertyBlock NormalizeShaderLabPropertyBlock(
			const MaterialPropertyBlock& source,
			const ShaderLabAssetDesc& desc)
		{
			NormalizedShaderLabPropertyBlock result{};
			result.PropertyBlock.OrphanValues = source.OrphanValues;

			for (const auto& [name, value] : source.Values)
			{
				bool foundInSchema = false;
				for (const MaterialPropertyDesc& property : desc.Properties)
				{
					if (property.Name == name)
					{
						foundInSchema = true;
						break;
					}
				}

				if (!foundInSchema)
				{
					result.PropertyBlock.OrphanValues[name] = value;
				}
			}

			for (const MaterialPropertyDesc& property : desc.Properties)
			{
				auto valueIt = source.Values.find(property.Name);
				auto orphanIt = result.PropertyBlock.OrphanValues.find(property.Name);

				if (valueIt != source.Values.end() && valueIt->second.ValueType == property.ValueType)
				{
					result.PropertyBlock.Values[property.Name] = valueIt->second;
					if (orphanIt != result.PropertyBlock.OrphanValues.end() &&
						orphanIt->second.ValueType == property.ValueType)
					{
						result.PropertyBlock.OrphanValues.erase(orphanIt);
					}
					continue;
				}

				if (valueIt != source.Values.end())
				{
					result.PropertyBlock.OrphanValues[property.Name] = valueIt->second;
					++result.MovedToOrphans;
					orphanIt = result.PropertyBlock.OrphanValues.find(property.Name);
				}

				if (orphanIt != result.PropertyBlock.OrphanValues.end() &&
					orphanIt->second.ValueType == property.ValueType)
				{
					result.PropertyBlock.Values[property.Name] = orphanIt->second;
					result.PropertyBlock.OrphanValues.erase(orphanIt);
					continue;
				}

				result.PropertyBlock.Values[property.Name] = property.DefaultValue;
				++result.FilledDefaults;
			}

			return result;
		}
	}

	VulkanResourceFactory::VulkanResourceFactory(VulkanContext& context, AssetManager& assetManager)
		: m_Context(context), m_AssetManager(assetManager)
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

	VulkanResourceFactory::ShaderBundle VulkanResourceFactory::BuildShaderLabPassBundle(
		AssetHandle handle,
		PassType passType)
	{
		if (!Asset::IsValidHandle(handle))
		{
			return {};
		}

		Ref<ShaderLabAsset> shaderLabAsset = m_AssetManager.GetShaderLabAsset(handle);
		if (!shaderLabAsset)
		{
			KITA_CORE_WARN("VulkanResourceFactory: shaderlab asset not found, handle={}", handle);
			return {};
		}

		const ShaderLabCompiledPass* matchedPass = nullptr;
		for (const ShaderLabCompiledPass& compiledPass : shaderLabAsset->CompiledPasses)
		{
			if (compiledPass.Type == passType)
			{
				matchedPass = &compiledPass;
				break;
			}
		}

		if (!matchedPass)
		{
			KITA_CORE_WARN(
				"VulkanResourceFactory: shaderlab asset has no compiled pass for requested type, handle={}, passType={}",
				handle,
				static_cast<int>(passType));
			return {};
		}

		ShaderBundle bundle{};
		bundle.VertexShader = BuildShader(
			shaderLabAsset->SourcePath.stem().string() + "_" + matchedPass->Name,
			matchedPass->VertexStage,
			VK_SHADER_STAGE_VERTEX_BIT,
			"VS");
		bundle.FragmentShader = BuildShader(
			shaderLabAsset->SourcePath.stem().string() + "_" + matchedPass->Name,
			matchedPass->FragmentStage,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			"FS");
		return bundle;
	}

	VulkanResourceFactory::ShaderBundle VulkanResourceFactory::BuildShaderBundleFromStageBinaries(
		const std::string& shaderName,
		const ShaderStageBinary& vertexStage,
		const ShaderStageBinary& fragmentStage)
	{
		ShaderBundle bundle{};
		bundle.VertexShader = BuildShader(
			shaderName,
			vertexStage,
			VK_SHADER_STAGE_VERTEX_BIT,
			"VS");
		bundle.FragmentShader = BuildShader(
			shaderName,
			fragmentStage,
			VK_SHADER_STAGE_FRAGMENT_BIT,
			"FS");
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
		// Rebuild from scratch so stale ShaderLab and legacy state cannot leak across refreshes.
		outMaterial.Destroy();
		MaterialPropertyBlock shaderLabResolvedPropertyBlock{};
		bool hasShaderLabResolvedPropertyBlock = false;

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

		outMaterial.ClearTextures();
		outMaterial.SetParams(BuildLegacyGpuParams(materialAsset));

		if (Asset::IsValidHandle(materialAsset.MaterialDefinitionHandle))
		{
			Ref<MaterialDefinitionAsset> shaderLabAsset = m_AssetManager.GetMaterialDefinitionAsset(materialAsset.MaterialDefinitionHandle);
			if (!shaderLabAsset || !shaderLabAsset->RuntimeLayout)
			{
				KITA_CORE_WARN(
					"VulkanResourceFactory: ShaderLab asset missing runtime layout, handle={}",
					materialAsset.MaterialDefinitionHandle);
			}
			else
			{
				const NormalizedShaderLabPropertyBlock normalizedPropertyBlock =
					NormalizeShaderLabPropertyBlock(materialAsset.PropertyBlock, *shaderLabAsset->Desc);
				if (normalizedPropertyBlock.FilledDefaults > 0 || normalizedPropertyBlock.MovedToOrphans > 0)
				{
					KITA_CORE_WARN(
						"VulkanResourceFactory: normalized ShaderLab material properties, shaderLabHandle={}, filledDefaults={}, movedToOrphans={}",
						materialAsset.MaterialDefinitionHandle,
						normalizedPropertyBlock.FilledDefaults,
						normalizedPropertyBlock.MovedToOrphans);
				}

				outMaterial.SetMaterialProperties(normalizedPropertyBlock.PropertyBlock);
				shaderLabResolvedPropertyBlock = normalizedPropertyBlock.PropertyBlock;
				hasShaderLabResolvedPropertyBlock = true;
				outMaterial.SetRuntimeLayout(*shaderLabAsset->RuntimeLayout);
				if (shaderLabAsset->LightingRuntime)
				{
					outMaterial.SetLightingRuntime(*shaderLabAsset->LightingRuntime);
				}

				std::vector<VulkanMaterial::PassRuntime> passes;
				passes.reserve(shaderLabAsset->CompiledPasses.size());
				for (const ShaderLabCompiledPass& compiledPass : shaderLabAsset->CompiledPasses)
				{
					if (compiledPass.Type == PassType::DeferredLighting)
					{
						KITA_CORE_WARN(
							"VulkanResourceFactory: skip DeferredLighting pass '{}' from surface ShaderLab '{}'.",
							compiledPass.Name,
							shaderLabAsset->SourcePath.string());
						continue;
					}

					VulkanMaterial::PassRuntime pass{};
					pass.Name = compiledPass.Name;
					pass.Type = compiledPass.Type;
					pass.RenderState = compiledPass.RenderState;
					pass.RenderGraph = compiledPass.RenderGraph;
					pass.VertexShader = BuildShader(
						shaderLabAsset->SourcePath.stem().string() + "_" + compiledPass.Name,
						compiledPass.VertexStage,
						VK_SHADER_STAGE_VERTEX_BIT,
						"VS");
					pass.FragmentShader = BuildShader(
						shaderLabAsset->SourcePath.stem().string() + "_" + compiledPass.Name,
						compiledPass.FragmentStage,
						VK_SHADER_STAGE_FRAGMENT_BIT,
						"FS");
					passes.push_back(std::move(pass));
				}
				outMaterial.SetPasses(std::move(passes));

				for (const auto& [propertyName, propertyValue] : normalizedPropertyBlock.PropertyBlock.Values)
				{
					if (propertyValue.ValueType != MaterialValueType::Texture2D &&
						propertyValue.ValueType != MaterialValueType::TextureCube)
					{
						continue;
					}

					if (!std::holds_alternative<AssetHandle>(propertyValue.Data))
					{
						continue;
					}

					const AssetHandle textureHandle = std::get<AssetHandle>(propertyValue.Data);
					if (!Asset::IsValidHandle(textureHandle))
					{
						continue;
					}

					Ref<VulkanTexture> texture = GetOrCreateTexture(textureHandle);
					if (texture)
					{
						outMaterial.SetResolvedTexture(propertyName, texture);
						MirrorShaderLabTextureToLegacySlot(outMaterial, propertyName, texture);
					}
				}
			}
		}

		// Fall back to the old shader pair if the ShaderLab path is missing a complete pass.
		if ((!outMaterial.GetVertexShader() || !outMaterial.GetFragmentShader()) &&
			Asset::IsValidHandle(materialAsset.ShaderHandle))
		{
			ShaderBundle shaderBundle = GetOrCreateShaderBundle(materialAsset.ShaderHandle);
			if (shaderBundle.IsValid())
			{
				outMaterial.SetVertexShader(shaderBundle.VertexShader);
				outMaterial.SetFragmentShader(shaderBundle.FragmentShader);
			}
		}

		if (!outMaterial.GetAlbedoTexture() && Asset::IsValidHandle(materialAsset.m_Textures.Albedo))
		{
			outMaterial.SetAlbedoTexture(GetOrCreateTexture(materialAsset.m_Textures.Albedo));
		}
		if (!outMaterial.GetNormalTexture() && Asset::IsValidHandle(materialAsset.m_Textures.Normal))
		{
			outMaterial.SetNormalTexture(GetOrCreateTexture(materialAsset.m_Textures.Normal));
		}
		if (!outMaterial.GetMetallicRoughnessTexture() && Asset::IsValidHandle(materialAsset.m_Textures.MetallicRoughness))
		{
			outMaterial.SetMetallicRoughnessTexture(GetOrCreateTexture(materialAsset.m_Textures.MetallicRoughness));
		}
		if (!outMaterial.GetAmbientOcclusionTexture() && Asset::IsValidHandle(materialAsset.m_Textures.AmbientOcclusion))
		{
			outMaterial.SetAmbientOcclusionTexture(GetOrCreateTexture(materialAsset.m_Textures.AmbientOcclusion));
		}
		if (!outMaterial.GetEmissiveTexture() && Asset::IsValidHandle(materialAsset.m_Textures.Emissive))
		{
			outMaterial.SetEmissiveTexture(GetOrCreateTexture(materialAsset.m_Textures.Emissive));
		}
		if (!outMaterial.GetOpacityTexture() && Asset::IsValidHandle(materialAsset.m_Textures.Opacity))
		{
			outMaterial.SetOpacityTexture(GetOrCreateTexture(materialAsset.m_Textures.Opacity));
		}

		if (!outMaterial.GetAlbedoTexture() && fallbackWhiteTexture)
			outMaterial.SetAlbedoTexture(fallbackWhiteTexture);
		if (!outMaterial.GetNormalTexture() && fallbackNormalTexture)
			outMaterial.SetNormalTexture(fallbackNormalTexture);
		if (!outMaterial.GetMetallicRoughnessTexture() && fallbackWhiteTexture)
			outMaterial.SetMetallicRoughnessTexture(fallbackWhiteTexture);
		if (!outMaterial.GetAmbientOcclusionTexture() && fallbackWhiteTexture)
			outMaterial.SetAmbientOcclusionTexture(fallbackWhiteTexture);
		if (!outMaterial.GetEmissiveTexture() && fallbackBlackTexture)
			outMaterial.SetEmissiveTexture(fallbackBlackTexture);
		if (!outMaterial.GetOpacityTexture() && fallbackWhiteTexture)
			outMaterial.SetOpacityTexture(fallbackWhiteTexture);

		// ShaderLab 材质的纹理属性可能和 legacy 命名槽位重名，
		// 上面的 fallback/legacy setter 会把 _Albedo / _Normal 之类重新覆盖成白图。
		// 最后再把 ShaderLab property block 解析出的运行时贴图覆写回去，确保 descriptor 绑定正确。
		if (hasShaderLabResolvedPropertyBlock)
		{
			for (const auto& [propertyName, propertyValue] : shaderLabResolvedPropertyBlock.Values)
			{
				if (propertyValue.ValueType != MaterialValueType::Texture2D &&
					propertyValue.ValueType != MaterialValueType::TextureCube)
				{
					continue;
				}

				if (!std::holds_alternative<AssetHandle>(propertyValue.Data))
				{
					continue;
				}

				const AssetHandle textureHandle = std::get<AssetHandle>(propertyValue.Data);
				if (!Asset::IsValidHandle(textureHandle))
				{
					continue;
				}

				Ref<VulkanTexture> texture = GetOrCreateTexture(textureHandle);
				if (texture)
				{
					outMaterial.SetResolvedTexture(propertyName, texture);
					MirrorShaderLabTextureToLegacySlot(outMaterial, propertyName, texture);
				}
			}
		}

		if (outMaterial.HasAnyTexture() || outMaterial.HasAnyShader())
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

	void VulkanResourceFactory::InvalidateMaterialDefinition(AssetHandle materialDefinitionHandle)
	{
		if (!Asset::IsValidHandle(materialDefinitionHandle))
		{
			return;
		}

		for (auto it = m_MaterialCache.begin(); it != m_MaterialCache.end();)
		{
			Ref<MaterialAsset> materialAsset = m_AssetManager.GetMaterialAsset(it->first);
			if (materialAsset && materialAsset->MaterialDefinitionHandle == materialDefinitionHandle)
			{
				it = m_MaterialCache.erase(it);
				continue;
			}

			++it;
		}
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

	Ref<VulkanShader> VulkanResourceFactory::BuildShader(
		const std::string& shaderName,
		const ShaderStageBinary& stageBinary,
		VkShaderStageFlagBits stage,
		const char* suffix)
	{
		if (!stageBinary.Valid())
		{
			return nullptr;
		}

		VulkanShader::CreateInfo createInfo{};
		createInfo.Name = shaderName + "_" + suffix;
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
