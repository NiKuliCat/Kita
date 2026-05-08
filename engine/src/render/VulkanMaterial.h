#pragma once

#include "core/Core.h"

#include <glm/glm.hpp>

namespace Kita {

	class VulkanTexture;

	class VulkanMaterial
	{
	public:
		VulkanMaterial() = default;
		~VulkanMaterial() = default;

		glm::vec4& GetBaseColor() { return m_BaseColor; }
		const glm::vec4& GetBaseColor() const { return m_BaseColor; }
		void SetBaseColor(const glm::vec4& color) { m_BaseColor = color; }

		const Ref<VulkanTexture>& GetAlbedoTexture() const { return m_AlbedoTexture; }
		void SetAlbedoTexture(const Ref<VulkanTexture>& texture) { m_AlbedoTexture = texture; }
		void ClearAlbedoTexture() { m_AlbedoTexture = nullptr; }

	private:
		glm::vec4 m_BaseColor = { 1.0f, 1.0f, 1.0f, 1.0f };
		Ref<VulkanTexture> m_AlbedoTexture = nullptr;
	};

}
