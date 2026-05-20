#pragma once

#include "core/Core.h"
#include "VulkanDescriptorSet.h"
#include "VulkanShader.h"
#include "VulkanUniformBuffer.h"
#include <glm/glm.hpp>
#include <vector>


namespace Kita {

	class VulkanTexture;
	class VulkanContext;

	struct alignas(16) MaterialGpuParams
	{
		glm::vec4 BaseColor = glm::vec4(1.0f);
		glm::vec4 Emissive = glm::vec4(0.0f);
		glm::vec4 SurfaceParams = glm::vec4(0.0f);
		// x = Metallic
		// y = Roughness
		// z = AmbientOcclusion
		// w = Opacity

		glm::vec4 MiscParams = glm::vec4(0.0f);
		// x = NormalScale
		// y = AlphaCutoff
		// z/w reserved

		glm::vec4 TextureFlags0 = glm::vec4(0.0f);
		// x = HasAlbedoTexture
		// y = HasNormalTexture
		// z = HasMetallicRoughnessTexture
		// w = HasAmbientOcclusionTexture

		glm::vec4 TextureFlags1 = glm::vec4(0.0f);
		// x = HasEmissiveTexture
		// y = HasOpacityTexture
		// z/w reserved
	};


	class VulkanMaterial
	{
	public:
		static constexpr uint32_t AlbedoBinding = 0;
		static constexpr uint32_t ParamsBinding = 1;
		static constexpr uint32_t NormalBinding = 2;
		static constexpr uint32_t MetallicRoughnessBinding = 3;
		static constexpr uint32_t AmbientOcclusionBinding = 4;
		static constexpr uint32_t EmissiveBinding = 5;
		static constexpr uint32_t OpacityBinding = 6;

		VulkanMaterial() = default;
		VulkanMaterial(const Ref<VulkanShader>& vertex, const Ref<VulkanShader>& frag, const Ref<VulkanTexture>& tex);
		~VulkanMaterial();

		glm::vec4& GetBaseColor() { return m_Params.BaseColor; }
		const glm::vec4& GetBaseColor() const { return m_Params.BaseColor; }
		void SetBaseColor(const glm::vec4& color)
		{
			m_Params.BaseColor = color;
			MarkDescriptorSetsDirty();
		}

		const Ref<VulkanTexture>& GetAlbedoTexture() const { return m_AlbedoTexture; }
		void SetAlbedoTexture(const Ref<VulkanTexture>& texture)
		{
			m_AlbedoTexture = texture;
			MarkDescriptorSetsDirty();
		}
		const Ref<VulkanTexture>& GetNormalTexture() const { return m_NormalTexture; }
		void SetNormalTexture(const Ref<VulkanTexture>& texture)
		{
			m_NormalTexture = texture;
			MarkDescriptorSetsDirty();
		}
		const Ref<VulkanTexture>& GetMetallicRoughnessTexture() const { return m_MetallicRoughnessTexture; }
		void SetMetallicRoughnessTexture(const Ref<VulkanTexture>& texture)
		{
			m_MetallicRoughnessTexture = texture;
			MarkDescriptorSetsDirty();
		}
		const Ref<VulkanTexture>& GetAmbientOcclusionTexture() const { return m_AmbientOcclusionTexture; }
		void SetAmbientOcclusionTexture(const Ref<VulkanTexture>& texture)
		{
			m_AmbientOcclusionTexture = texture;
			MarkDescriptorSetsDirty();
		}
		const Ref<VulkanTexture>& GetEmissiveTexture() const { return m_EmissiveTexture; }
		void SetEmissiveTexture(const Ref<VulkanTexture>& texture)
		{
			m_EmissiveTexture = texture;
			MarkDescriptorSetsDirty();
		}
		const Ref<VulkanTexture>& GetOpacityTexture() const { return m_OpacityTexture; }
		void SetOpacityTexture(const Ref<VulkanTexture>& texture)
		{
			m_OpacityTexture = texture;
			MarkDescriptorSetsDirty();
		}
		void SetFallbackTextures(
			const Ref<VulkanTexture>& whiteTexture,
			const Ref<VulkanTexture>& blackTexture,
			const Ref<VulkanTexture>& normalTexture)
		{
			m_FallbackWhiteTexture = whiteTexture;
			m_FallbackBlackTexture = blackTexture;
			m_FallbackNormalTexture = normalTexture;
			MarkDescriptorSetsDirty();
		}

		void SetVertexShader(const Ref<VulkanShader>& vertex) { m_VertexShader = vertex; }
		const Ref<VulkanShader>& GetVertexShader() const { return m_VertexShader; }


		void SetFragmentShader(const Ref<VulkanShader>& frag) { m_FragmentShader = frag; }
		const Ref<VulkanShader>& GetFragmentShader() const { return m_FragmentShader; }

		void SetParams(const MaterialGpuParams& params);
		const MaterialGpuParams& GetParams() const { return m_Params; }
		void UpdateParamBuffer(uint32_t frameIndex);

		void ClearAlbedoTexture()
		{
			m_AlbedoTexture = nullptr;
			MarkDescriptorSetsDirty();
		}
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

		MaterialGpuParams m_Params;
		Ref<VulkanShader> m_VertexShader = nullptr;
		Ref<VulkanShader> m_FragmentShader = nullptr;

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
