#pragma once

#include "core/Core.h"
#include "VulkanDescriptorSet.h"
#include "VulkanShader.h"
#include "VulkanUniformBuffer.h"
#include "asset/ShaderLabAsset.h"
#include "render/material/MaterialRuntimeLayout.h"
#include "render/shaderLab/ShaderLabType.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

namespace Kita {

	class VulkanTexture;
	class VulkanContext;

	// Legacy GPU parameter block kept for preview and old material paths.
	struct alignas(16) MaterialGpuParams
	{
		glm::vec4 BaseColor = glm::vec4(1.0f);
		glm::vec4 Emissive = glm::vec4(0.0f);
		glm::vec4 SurfaceParams = glm::vec4(0.0f);
		glm::vec4 MiscParams = glm::vec4(0.0f);
		glm::vec4 TextureFlags0 = glm::vec4(0.0f);
		glm::vec4 TextureFlags1 = glm::vec4(0.0f);
	};

	class VulkanMaterial
	{
	public:
		struct PassRuntime
		{
			std::string Name;
			PassType Type = PassType::Unknown;
			ShaderLabRenderStateDesc RenderState{};
			ShaderLabRenderGraphDesc RenderGraph{};
			Ref<VulkanShader> VertexShader = nullptr;
			Ref<VulkanShader> FragmentShader = nullptr;
		};

	public:
		VulkanMaterial() = default;
		~VulkanMaterial();

		// Compatibility accessors for old callers that expect a single VS/FS pair.
		void SetVertexShader(const Ref<VulkanShader>& vertex);
		const Ref<VulkanShader>& GetVertexShader() const;

		void SetFragmentShader(const Ref<VulkanShader>& frag);
		const Ref<VulkanShader>& GetFragmentShader() const;

		// Compatibility texture slots used by the legacy material path.
		const Ref<VulkanTexture>& GetAlbedoTexture() const;
		void SetAlbedoTexture(const Ref<VulkanTexture>& texture);
		const Ref<VulkanTexture>& GetNormalTexture() const;
		void SetNormalTexture(const Ref<VulkanTexture>& texture);
		const Ref<VulkanTexture>& GetMetallicRoughnessTexture() const;
		void SetMetallicRoughnessTexture(const Ref<VulkanTexture>& texture);
		const Ref<VulkanTexture>& GetAmbientOcclusionTexture() const;
		void SetAmbientOcclusionTexture(const Ref<VulkanTexture>& texture);
		const Ref<VulkanTexture>& GetEmissiveTexture() const;
		void SetEmissiveTexture(const Ref<VulkanTexture>& texture);
		const Ref<VulkanTexture>& GetOpacityTexture() const;
		void SetOpacityTexture(const Ref<VulkanTexture>& texture);

		void SetFallbackTextures(
			const Ref<VulkanTexture>& whiteTexture,
			const Ref<VulkanTexture>& blackTexture,
			const Ref<VulkanTexture>& normalTexture);

		void SetMaterialProperties(const MaterialPropertyBlock& propertyBlock);
		const MaterialPropertyBlock& GetMaterialProperties() const { return m_PropertyBlock; }

		// Converts legacy constant-buffer data into the new property container.
		void SetParams(const MaterialGpuParams& params);
		const MaterialGpuParams& GetParams() const { return m_LegacyParams; }

		void SetRuntimeLayout(const MaterialRuntimeLayout& layout);
		const MaterialRuntimeLayout* GetRuntimeLayout() const { return m_RuntimeLayoutValid ? &m_RuntimeLayout : nullptr; }
		// 缓存材质参与统一 DeferredLighting 所需的 shading model 元数据。
		void SetLightingRuntime(const ShaderLabLightingRuntimeDesc& lightingRuntime);
		const ShaderLabLightingRuntimeDesc& GetLightingRuntime() const { return m_LightingRuntime; }

		void SetPasses(std::vector<PassRuntime> passes);
		const PassRuntime* FindPass(PassType passType) const;
		const std::vector<PassRuntime>& GetPasses() const { return m_Passes; }
		bool HasAnyShader() const;

		void SetResolvedTexture(const std::string& propertyName, const Ref<VulkanTexture>& texture);
		const Ref<VulkanTexture>& GetResolvedTexture(const std::string& propertyName) const;
		void ClearTextures();

		void InitDescriptors(VulkanContext& context, uint32_t framesInFlight);
		void UpdateDescriptorSets();
		void UpdateDescriptorSet(uint32_t frameIndex);
		void MarkDescriptorSetsDirty();
		void EnsureDescriptors(VulkanContext& context, uint32_t framesInFlight);
		void Destroy();

		const VulkanDescriptorSet& GetDescriptorSet(uint32_t frameIndex) const;
		bool HasDescriptorSets() const { return !m_DescriptorSets.empty(); }
		bool IsDescriptorSetDirty(uint32_t frameIndex) const;
		bool HasAnyTexture() const;

	private:
		void DestroyDescriptorResources();
		void RebuildUniformData();
		Ref<VulkanTexture> ResolveTextureForBinding(const MaterialResourceBinding& binding) const;
		const Ref<VulkanTexture>& GetFallbackTextureForBinding(const MaterialResourceBinding& binding) const;
		const Ref<VulkanTexture>& GetLegacyNamedTexture(const std::string& propertyName) const;
		static bool IsTextureProperty(MaterialValueType valueType);

	private:
		std::vector<PassRuntime> m_Passes;
		MaterialPropertyBlock m_PropertyBlock{};
		MaterialRuntimeLayout m_RuntimeLayout{};
		bool m_RuntimeLayoutValid = false;
		ShaderLabLightingRuntimeDesc m_LightingRuntime{};
		std::vector<uint8_t> m_UniformData;
		MaterialGpuParams m_LegacyParams{};

		// Property name to runtime texture object.
		std::unordered_map<std::string, Ref<VulkanTexture>> m_ResolvedTextures;

		// Cached legacy texture slots.
		Ref<VulkanTexture> m_AlbedoTexture = nullptr;
		Ref<VulkanTexture> m_NormalTexture = nullptr;
		Ref<VulkanTexture> m_MetallicRoughnessTexture = nullptr;
		Ref<VulkanTexture> m_AmbientOcclusionTexture = nullptr;
		Ref<VulkanTexture> m_EmissiveTexture = nullptr;
		Ref<VulkanTexture> m_OpacityTexture = nullptr;

		Ref<VulkanTexture> m_FallbackWhiteTexture = nullptr;
		Ref<VulkanTexture> m_FallbackBlackTexture = nullptr;
		Ref<VulkanTexture> m_FallbackNormalTexture = nullptr;

		VulkanContext* m_Context = nullptr;
		std::vector<VulkanUniformBuffer> m_ParamUBOs;
		std::vector<VulkanDescriptorSet> m_DescriptorSets;
		std::vector<uint8_t> m_DescriptorDirtyFlags;
	};

}
