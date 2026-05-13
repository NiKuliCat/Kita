#pragma once

#include "core/Core.h"
#include "VulkanDescriptorSet.h"
#include "VulkanShader.h"
#include <glm/glm.hpp>
#include <vector>


namespace Kita {

	class VulkanTexture;
	class VulkanContext;

	class VulkanMaterial
	{
	public:
		VulkanMaterial() = default;
		VulkanMaterial(const Ref<VulkanShader>& vertex, const Ref<VulkanShader>& frag, const Ref<VulkanTexture>& tex);
		~VulkanMaterial();

		glm::vec4& GetBaseColor() { return m_BaseColor; }
		const glm::vec4& GetBaseColor() const { return m_BaseColor; }
		void SetBaseColor(const glm::vec4& color) { m_BaseColor = color; }

		const Ref<VulkanTexture>& GetAlbedoTexture() const { return m_AlbedoTexture; }
		void SetAlbedoTexture(const Ref<VulkanTexture>& texture) { m_AlbedoTexture = texture; }

		void SetVertexShader(const Ref<VulkanShader>& vertex) { m_VertexShader = vertex; }
		const Ref<VulkanShader>& GetVertexShader() const { return m_VertexShader; }


		void SetFragmentShader(const Ref<VulkanShader>& frag) { m_FragmentShader = frag; }
		const Ref<VulkanShader>& GetFragmentShader() const { return m_FragmentShader; }

		void ClearAlbedoTexture() { m_AlbedoTexture = nullptr; }

		void InitDescriptors(VulkanContext& context, uint32_t framesInFlight);
		void UpdateDescriptorSets();
		void Destroy();

		const VulkanDescriptorSet& GetDescriptorSet(uint32_t frameIndex) const;
		bool HasDescriptorSets() const { return !m_DescriptorSets.empty(); }

	private:
		glm::vec4 m_BaseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		Ref<VulkanShader> m_VertexShader = nullptr;
		Ref<VulkanShader> m_FragmentShader = nullptr;
		Ref<VulkanTexture> m_AlbedoTexture = nullptr;
		VulkanContext* m_Context = nullptr;
		std::vector<VulkanDescriptorSet> m_DescriptorSets;
	};

}
