#include "kita_pch.h"
#include "VulkanMaterial.h"

#include "VulkanContext.h"
#include "VulkanTexture.h"
#include "core/Log.h"

#include <cstring>

namespace Kita {

	namespace
	{
		const Ref<VulkanTexture>& FirstValidTexture(
			std::initializer_list<const Ref<VulkanTexture>*> candidates)
		{
			for (const Ref<VulkanTexture>* candidate : candidates)
			{
				if (candidate && *candidate && (*candidate)->IsValid())
					return *candidate;
			}

			static const Ref<VulkanTexture> s_NullTexture = nullptr;
			return s_NullTexture;
		}

		template<typename T>
		void CopyUniformValue(std::vector<uint8_t>& buffer, uint32_t offset, const T& value)
		{
			if (offset + sizeof(T) > buffer.size())
				return;

			std::memcpy(buffer.data() + offset, &value, sizeof(T));
		}
	}

	VulkanMaterial::~VulkanMaterial()
	{
		Destroy();
	}

	void VulkanMaterial::SetVertexShader(const Ref<VulkanShader>& vertex)
	{
		if (m_Passes.empty())
		{
			PassRuntime pass{};
			pass.Name = "Legacy";
			pass.Type = PassType::GBuffer;
			pass.VertexShader = vertex;
			m_Passes.push_back(pass);
		}
		else
		{
			m_Passes.front().VertexShader = vertex;
		}
	}

	const Ref<VulkanShader>& VulkanMaterial::GetVertexShader() const
	{
		static const Ref<VulkanShader> s_NullShader = nullptr;
		for (const PassRuntime& pass : m_Passes)
		{
			if (pass.VertexShader)
				return pass.VertexShader;
		}
		return s_NullShader;
	}

	void VulkanMaterial::SetFragmentShader(const Ref<VulkanShader>& frag)
	{
		if (m_Passes.empty())
		{
			PassRuntime pass{};
			pass.Name = "Legacy";
			pass.Type = PassType::GBuffer;
			pass.FragmentShader = frag;
			m_Passes.push_back(pass);
		}
		else
		{
			m_Passes.front().FragmentShader = frag;
		}
	}

	const Ref<VulkanShader>& VulkanMaterial::GetFragmentShader() const
	{
		static const Ref<VulkanShader> s_NullShader = nullptr;
		for (const PassRuntime& pass : m_Passes)
		{
			if (pass.FragmentShader)
				return pass.FragmentShader;
		}
		return s_NullShader;
	}

	const Ref<VulkanTexture>& VulkanMaterial::GetAlbedoTexture() const
	{
		return m_AlbedoTexture;
	}

	void VulkanMaterial::SetAlbedoTexture(const Ref<VulkanTexture>& texture)
	{
		m_AlbedoTexture = texture;
		m_ResolvedTextures["_Albedo"] = texture;
		MarkDescriptorSetsDirty();
	}

	const Ref<VulkanTexture>& VulkanMaterial::GetNormalTexture() const
	{
		return m_NormalTexture;
	}

	void VulkanMaterial::SetNormalTexture(const Ref<VulkanTexture>& texture)
	{
		m_NormalTexture = texture;
		m_ResolvedTextures["_Normal"] = texture;
		MarkDescriptorSetsDirty();
	}

	const Ref<VulkanTexture>& VulkanMaterial::GetMetallicRoughnessTexture() const
	{
		return m_MetallicRoughnessTexture;
	}

	void VulkanMaterial::SetMetallicRoughnessTexture(const Ref<VulkanTexture>& texture)
	{
		m_MetallicRoughnessTexture = texture;
		m_ResolvedTextures["_MetallicRoughness"] = texture;
		MarkDescriptorSetsDirty();
	}

	const Ref<VulkanTexture>& VulkanMaterial::GetAmbientOcclusionTexture() const
	{
		return m_AmbientOcclusionTexture;
	}

	void VulkanMaterial::SetAmbientOcclusionTexture(const Ref<VulkanTexture>& texture)
	{
		m_AmbientOcclusionTexture = texture;
		m_ResolvedTextures["_AmbientOcclusionTexture"] = texture;
		MarkDescriptorSetsDirty();
	}

	const Ref<VulkanTexture>& VulkanMaterial::GetEmissiveTexture() const
	{
		return m_EmissiveTexture;
	}

	void VulkanMaterial::SetEmissiveTexture(const Ref<VulkanTexture>& texture)
	{
		m_EmissiveTexture = texture;
		m_ResolvedTextures["_EmissiveTexture"] = texture;
		MarkDescriptorSetsDirty();
	}

	const Ref<VulkanTexture>& VulkanMaterial::GetOpacityTexture() const
	{
		return m_OpacityTexture;
	}

	void VulkanMaterial::SetOpacityTexture(const Ref<VulkanTexture>& texture)
	{
		m_OpacityTexture = texture;
		m_ResolvedTextures["_OpacityTexture"] = texture;
		MarkDescriptorSetsDirty();
	}

	void VulkanMaterial::SetFallbackTextures(
		const Ref<VulkanTexture>& whiteTexture,
		const Ref<VulkanTexture>& blackTexture,
		const Ref<VulkanTexture>& normalTexture)
	{
		m_FallbackWhiteTexture = whiteTexture;
		m_FallbackBlackTexture = blackTexture;
		m_FallbackNormalTexture = normalTexture;
		MarkDescriptorSetsDirty();
	}

	void VulkanMaterial::SetMaterialProperties(const MaterialPropertyBlock& propertyBlock)
	{
		m_PropertyBlock = propertyBlock;
		RebuildUniformData();
		MarkDescriptorSetsDirty();
	}

	void VulkanMaterial::SetParams(const MaterialGpuParams& params)
	{
		m_LegacyParams = params;

		// Mirror the legacy constant buffer into the generic property container.
		MaterialPropertyBlock block{};

		MaterialPropertyValue baseColor{};
		baseColor.ValueType = MaterialValueType::Color;
		baseColor.Data = params.BaseColor;
		block.Values["_BaseColor"] = baseColor;

		MaterialPropertyValue emissive{};
		emissive.ValueType = MaterialValueType::Float3;
		emissive.Data = glm::vec3(params.Emissive);
		block.Values["_Emissive"] = emissive;

		MaterialPropertyValue metallic{};
		metallic.ValueType = MaterialValueType::Float;
		metallic.Data = params.SurfaceParams.x;
		block.Values["_Metallic"] = metallic;

		MaterialPropertyValue roughness{};
		roughness.ValueType = MaterialValueType::Float;
		roughness.Data = params.SurfaceParams.y;
		block.Values["_Roughness"] = roughness;

		MaterialPropertyValue ao{};
		ao.ValueType = MaterialValueType::Float;
		ao.Data = params.SurfaceParams.z;
		block.Values["_AmbientOcclusion"] = ao;

		MaterialPropertyValue opacity{};
		opacity.ValueType = MaterialValueType::Float;
		opacity.Data = params.SurfaceParams.w;
		block.Values["_Opacity"] = opacity;

		MaterialPropertyValue normalScale{};
		normalScale.ValueType = MaterialValueType::Float;
		normalScale.Data = params.MiscParams.x;
		block.Values["_NormalScale"] = normalScale;

		MaterialPropertyValue alphaCutoff{};
		alphaCutoff.ValueType = MaterialValueType::Float;
		alphaCutoff.Data = params.MiscParams.y;
		block.Values["_AlphaCutoff"] = alphaCutoff;

		m_PropertyBlock = std::move(block);
		RebuildUniformData();
		MarkDescriptorSetsDirty();
	}

	void VulkanMaterial::SetRuntimeLayout(const MaterialRuntimeLayout& layout)
	{
		m_RuntimeLayout = layout;
		m_RuntimeLayoutValid = true;
		RebuildUniformData();
		MarkDescriptorSetsDirty();
	}

	void VulkanMaterial::SetLightingRuntime(const ShaderLabLightingRuntimeDesc& lightingRuntime)
	{
		m_LightingRuntime = lightingRuntime;
		RebuildUniformData();
		MarkDescriptorSetsDirty();
	}

	void VulkanMaterial::SetPasses(std::vector<PassRuntime> passes)
	{
		m_Passes = std::move(passes);
	}

	const VulkanMaterial::PassRuntime* VulkanMaterial::FindPass(PassType passType) const
	{
		for (const PassRuntime& pass : m_Passes)
		{
			if (pass.Type == passType)
				return &pass;
		}

		for (const PassRuntime& pass : m_Passes)
		{
			if (pass.VertexShader && pass.FragmentShader)
				return &pass;
		}

		return nullptr;
	}

	bool VulkanMaterial::HasAnyShader() const
	{
		for (const PassRuntime& pass : m_Passes)
		{
			if (pass.VertexShader || pass.FragmentShader)
				return true;
		}
		return false;
	}

	void VulkanMaterial::SetResolvedTexture(const std::string& propertyName, const Ref<VulkanTexture>& texture)
	{
		m_ResolvedTextures[propertyName] = texture;
		MarkDescriptorSetsDirty();
	}

	const Ref<VulkanTexture>& VulkanMaterial::GetResolvedTexture(const std::string& propertyName) const
	{
		auto it = m_ResolvedTextures.find(propertyName);
		if (it != m_ResolvedTextures.end())
			return it->second;

		static const Ref<VulkanTexture> s_NullTexture = nullptr;
		return s_NullTexture;
	}

	void VulkanMaterial::ClearTextures()
	{
		m_ResolvedTextures.clear();
		m_AlbedoTexture = nullptr;
		m_NormalTexture = nullptr;
		m_MetallicRoughnessTexture = nullptr;
		m_AmbientOcclusionTexture = nullptr;
		m_EmissiveTexture = nullptr;
		m_OpacityTexture = nullptr;
		MarkDescriptorSetsDirty();
	}

	void VulkanMaterial::InitDescriptors(VulkanContext& context, uint32_t framesInFlight)
	{
		DestroyDescriptorResources();

		m_Context = &context;
		const uint32_t descriptorCount = std::max(1u, framesInFlight);
		m_ParamUBOs.resize(descriptorCount);
		m_DescriptorSets.resize(descriptorCount);
		m_DescriptorDirtyFlags.assign(descriptorCount, 1);

		VulkanDescriptorSet::CreateInfo descInfo{};
		descInfo.Name = "Material_Set";
		if (m_RuntimeLayoutValid)
		{
			descInfo.Bindings.push_back({
				m_RuntimeLayout.UniformBinding,
				VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				1,
				VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT
			});

			for (const MaterialResourceBinding& binding : m_RuntimeLayout.ResourceBindings)
			{
				descInfo.Bindings.push_back({
					binding.Binding,
					VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
					1,
					VK_SHADER_STAGE_FRAGMENT_BIT
				});
			}
		}
		else
		{
			// Keep the old fixed descriptor layout alive until renderer migration is done.
			descInfo.Bindings = {
				{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
				{ 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
				{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
				{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
				{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
				{ 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
				{ 6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			};
		}

		const uint32_t uniformBufferSize = m_RuntimeLayoutValid
			? std::max(16u, m_RuntimeLayout.UniformBufferSize)
			: static_cast<uint32_t>(sizeof(MaterialGpuParams));

		for (uint32_t i = 0; i < descriptorCount; ++i)
		{
			m_ParamUBOs[i].Init(context, uniformBufferSize, "MaterialParamsUBO_" + std::to_string(i));
			VulkanDescriptorSet::CreateInfo perFrameDescInfo = descInfo;
			perFrameDescInfo.Name += "_" + std::to_string(i);
			m_DescriptorSets[i].Init(context, perFrameDescInfo);
		}

		UpdateDescriptorSets();
	}

	void VulkanMaterial::UpdateDescriptorSets()
	{
		if (!m_Context)
			return;

		for (uint32_t i = 0; i < static_cast<uint32_t>(m_DescriptorSets.size()); ++i)
		{
			UpdateDescriptorSet(i);
		}
	}

	void VulkanMaterial::UpdateDescriptorSet(uint32_t frameIndex)
	{
		if (!m_Context || frameIndex >= m_DescriptorSets.size())
			return;

		if (frameIndex < m_ParamUBOs.size() && m_ParamUBOs[frameIndex].IsValid())
		{
			if (m_RuntimeLayoutValid)
			{
				if (!m_UniformData.empty())
				{
					m_ParamUBOs[frameIndex].SetData(m_UniformData.data(), static_cast<uint32_t>(m_UniformData.size()));
				}
			}
			else
			{
				m_ParamUBOs[frameIndex].SetData(&m_LegacyParams, static_cast<uint32_t>(sizeof(MaterialGpuParams)));
			}

			m_DescriptorSets[frameIndex].WriteUniformBuffer(
				m_RuntimeLayoutValid ? m_RuntimeLayout.UniformBinding : 1,
				m_ParamUBOs[frameIndex]);
		}

		if (m_RuntimeLayoutValid)
		{
			for (const MaterialResourceBinding& binding : m_RuntimeLayout.ResourceBindings)
			{
				Ref<VulkanTexture> texture = ResolveTextureForBinding(binding);
				if (!texture || !texture->IsValid())
				{
					const Ref<VulkanTexture>& fallbackTexture = GetFallbackTextureForBinding(binding);
					texture = fallbackTexture;
				}

				KITA_CORE_ASSERT(texture && texture->IsValid(), "VulkanMaterial requires a valid fallback texture");
				m_DescriptorSets[frameIndex].WriteImageSampler(binding.Binding, texture->GetDescriptorInfo());
			}
		}
		else
		{
			const Ref<VulkanTexture>& albedoTexture = FirstValidTexture({ &m_AlbedoTexture, &m_FallbackWhiteTexture });
			const Ref<VulkanTexture>& normalTexture = FirstValidTexture({ &m_NormalTexture, &m_FallbackNormalTexture, &m_FallbackWhiteTexture });
			const Ref<VulkanTexture>& metallicRoughnessTexture = FirstValidTexture({ &m_MetallicRoughnessTexture, &m_FallbackWhiteTexture });
			const Ref<VulkanTexture>& ambientOcclusionTexture = FirstValidTexture({ &m_AmbientOcclusionTexture, &m_FallbackWhiteTexture });
			const Ref<VulkanTexture>& emissiveTexture = FirstValidTexture({ &m_EmissiveTexture, &m_FallbackBlackTexture, &m_FallbackWhiteTexture });
			const Ref<VulkanTexture>& opacityTexture = FirstValidTexture({ &m_OpacityTexture, &m_FallbackWhiteTexture });

			KITA_CORE_ASSERT(albedoTexture && albedoTexture->IsValid(), "VulkanMaterial requires a valid albedo fallback texture");
			KITA_CORE_ASSERT(normalTexture && normalTexture->IsValid(), "VulkanMaterial requires a valid normal fallback texture");
			KITA_CORE_ASSERT(metallicRoughnessTexture && metallicRoughnessTexture->IsValid(), "VulkanMaterial requires a valid metallic-roughness fallback texture");
			KITA_CORE_ASSERT(ambientOcclusionTexture && ambientOcclusionTexture->IsValid(), "VulkanMaterial requires a valid ambient-occlusion fallback texture");
			KITA_CORE_ASSERT(emissiveTexture && emissiveTexture->IsValid(), "VulkanMaterial requires a valid emissive fallback texture");
			KITA_CORE_ASSERT(opacityTexture && opacityTexture->IsValid(), "VulkanMaterial requires a valid opacity fallback texture");

			m_DescriptorSets[frameIndex].WriteImageSampler(0, albedoTexture->GetDescriptorInfo());
			m_DescriptorSets[frameIndex].WriteImageSampler(2, normalTexture->GetDescriptorInfo());
			m_DescriptorSets[frameIndex].WriteImageSampler(3, metallicRoughnessTexture->GetDescriptorInfo());
			m_DescriptorSets[frameIndex].WriteImageSampler(4, ambientOcclusionTexture->GetDescriptorInfo());
			m_DescriptorSets[frameIndex].WriteImageSampler(5, emissiveTexture->GetDescriptorInfo());
			m_DescriptorSets[frameIndex].WriteImageSampler(6, opacityTexture->GetDescriptorInfo());
		}

		if (frameIndex < m_DescriptorDirtyFlags.size())
		{
			m_DescriptorDirtyFlags[frameIndex] = 0;
		}
	}

	void VulkanMaterial::MarkDescriptorSetsDirty()
	{
		if (m_DescriptorDirtyFlags.empty())
			return;

		std::fill(m_DescriptorDirtyFlags.begin(), m_DescriptorDirtyFlags.end(), static_cast<uint8_t>(1));
	}

	void VulkanMaterial::EnsureDescriptors(VulkanContext& context, uint32_t framesInFlight)
	{
		const uint32_t descriptorCount = std::max(1u, framesInFlight);
		if (m_Context != &context || m_DescriptorSets.size() != descriptorCount)
		{
			InitDescriptors(context, descriptorCount);
			return;
		}

		if (m_DescriptorSets.empty())
		{
			InitDescriptors(context, descriptorCount);
		}
	}

	bool VulkanMaterial::HasAnyTexture() const
	{
		for (const auto& [_, texture] : m_ResolvedTextures)
		{
			if (texture && texture->IsValid())
				return true;
		}

		return
			(m_AlbedoTexture && m_AlbedoTexture->IsValid()) ||
			(m_NormalTexture && m_NormalTexture->IsValid()) ||
			(m_MetallicRoughnessTexture && m_MetallicRoughnessTexture->IsValid()) ||
			(m_AmbientOcclusionTexture && m_AmbientOcclusionTexture->IsValid()) ||
			(m_EmissiveTexture && m_EmissiveTexture->IsValid()) ||
			(m_OpacityTexture && m_OpacityTexture->IsValid());
	}

	bool VulkanMaterial::IsDescriptorSetDirty(uint32_t frameIndex) const
	{
		if (frameIndex >= m_DescriptorDirtyFlags.size())
			return false;

		return m_DescriptorDirtyFlags[frameIndex] != 0;
	}

	void VulkanMaterial::DestroyDescriptorResources()
	{
		for (auto& ubo : m_ParamUBOs)
			ubo.Destroy();
		m_ParamUBOs.clear();

		for (auto& descriptorSet : m_DescriptorSets)
			descriptorSet.Destroy();
		m_DescriptorSets.clear();
		m_DescriptorDirtyFlags.clear();

		m_Context = nullptr;
	}

	void VulkanMaterial::Destroy()
	{
		DestroyDescriptorResources();
		m_Passes.clear();
		m_PropertyBlock = {};
		m_RuntimeLayout = {};
		m_RuntimeLayoutValid = false;
		m_LightingRuntime = {};
		m_UniformData.clear();
		m_LegacyParams = {};
		m_ResolvedTextures.clear();

		m_AlbedoTexture = nullptr;
		m_NormalTexture = nullptr;
		m_MetallicRoughnessTexture = nullptr;
		m_AmbientOcclusionTexture = nullptr;
		m_EmissiveTexture = nullptr;
		m_OpacityTexture = nullptr;
		m_FallbackWhiteTexture = nullptr;
		m_FallbackBlackTexture = nullptr;
		m_FallbackNormalTexture = nullptr;
	}

	const VulkanDescriptorSet& VulkanMaterial::GetDescriptorSet(uint32_t frameIndex) const
	{
		KITA_CORE_ASSERT(!m_DescriptorSets.empty(), "VulkanMaterial descriptor sets are empty");
		KITA_CORE_ASSERT(frameIndex < m_DescriptorSets.size(), "VulkanMaterial frame index is out of range");
		return m_DescriptorSets[frameIndex];
	}

	void VulkanMaterial::RebuildUniformData()
	{
		if (!m_RuntimeLayoutValid)
		{
			m_UniformData.clear();
			return;
		}

		m_UniformData.assign(m_RuntimeLayout.UniformBufferSize, 0);

		for (const MaterialUniformMember& member : m_RuntimeLayout.UniformMembers)
		{
			if (member.Name == MaterialSystemFieldShadingModelID)
			{
				const int32_t shadingModelID = static_cast<int32_t>(m_LightingRuntime.ShadingModelID);
				CopyUniformValue(m_UniformData, member.Offset, shadingModelID);
				continue;
			}

			if (member.Name == MaterialSystemFieldCustomDataCount)
			{
				const int32_t customDataCount = static_cast<int32_t>(m_LightingRuntime.CustomDataCount);
				CopyUniformValue(m_UniformData, member.Offset, customDataCount);
				continue;
			}

			auto valueIt = m_PropertyBlock.Values.find(member.Name);
			if (valueIt == m_PropertyBlock.Values.end())
				continue;

			const MaterialPropertyValue& value = valueIt->second;
			switch (value.ValueType)
			{
			case MaterialValueType::Bool:
			{
				const int boolValue = std::get<bool>(value.Data) ? 1 : 0;
				CopyUniformValue(m_UniformData, member.Offset, boolValue);
				break;
			}
			case MaterialValueType::Int:
				CopyUniformValue(m_UniformData, member.Offset, std::get<int32_t>(value.Data));
				break;
			case MaterialValueType::Float:
				CopyUniformValue(m_UniformData, member.Offset, std::get<float>(value.Data));
				break;
			case MaterialValueType::Float2:
				CopyUniformValue(m_UniformData, member.Offset, std::get<glm::vec2>(value.Data));
				break;
			case MaterialValueType::Float3:
				CopyUniformValue(m_UniformData, member.Offset, std::get<glm::vec3>(value.Data));
				break;
			case MaterialValueType::Float4:
			case MaterialValueType::Color:
				CopyUniformValue(m_UniformData, member.Offset, std::get<glm::vec4>(value.Data));
				break;
			default:
				break;
			}
		}
	}

	Ref<VulkanTexture> VulkanMaterial::ResolveTextureForBinding(const MaterialResourceBinding& binding) const
	{
		auto textureIt = m_ResolvedTextures.find(binding.Name);
		if (textureIt != m_ResolvedTextures.end() && textureIt->second && textureIt->second->IsValid())
			return textureIt->second;

		const Ref<VulkanTexture>& legacyTexture = GetLegacyNamedTexture(binding.Name);
		if (legacyTexture && legacyTexture->IsValid())
			return legacyTexture;

		return nullptr;
	}

	const Ref<VulkanTexture>& VulkanMaterial::GetFallbackTextureForBinding(const MaterialResourceBinding& binding) const
	{
		if (binding.ValueType == MaterialValueType::TextureCube)
		{
			return FirstValidTexture({ &m_FallbackBlackTexture, &m_FallbackWhiteTexture });
		}

		if (binding.Name.find("Normal") != std::string::npos || binding.Name.find("normal") != std::string::npos)
		{
			return FirstValidTexture({ &m_FallbackNormalTexture, &m_FallbackWhiteTexture });
		}

		if (binding.Name.find("Emissive") != std::string::npos || binding.Name.find("emissive") != std::string::npos)
		{
			return FirstValidTexture({ &m_FallbackBlackTexture, &m_FallbackWhiteTexture });
		}

		return FirstValidTexture({ &m_FallbackWhiteTexture, &m_FallbackBlackTexture });
	}

	const Ref<VulkanTexture>& VulkanMaterial::GetLegacyNamedTexture(const std::string& propertyName) const
	{
		if (propertyName == "_Albedo") return m_AlbedoTexture;
		if (propertyName == "_Normal") return m_NormalTexture;
		if (propertyName == "_MetallicRoughness") return m_MetallicRoughnessTexture;
		if (propertyName == "_AmbientOcclusionTexture") return m_AmbientOcclusionTexture;
		if (propertyName == "_EmissiveTexture") return m_EmissiveTexture;

		// 兼容新的 ShaderLab GBuffer 命名。
		if (propertyName == "_EmissionMask") return m_EmissiveTexture;

		if (propertyName == "_OpacityTexture") return m_OpacityTexture;

		static const Ref<VulkanTexture> s_NullTexture = nullptr;
		return s_NullTexture;
	}

	bool VulkanMaterial::IsTextureProperty(MaterialValueType valueType)
	{
		return valueType == MaterialValueType::Texture2D || valueType == MaterialValueType::TextureCube;
	}

}
