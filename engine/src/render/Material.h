#pragma once

#include "Shader.h"
#include "Texture.h"
namespace Kita {


	class Material
	{
	public:
		Material() = default;
		Material(const Ref<Shader>& shader, const Ref<Texture>& tex);

		Ref<Shader>& GetShader() { return m_Shader; }
		const Ref<Shader>& GetShader() const { return m_Shader; }
		void SetShader(const Ref<Shader>& shader) { m_Shader = shader; }

		const Ref<Texture>& GetAlbedoTexture() const { return m_AlbedoTex; }
		void SetAlbedoTexture(const Ref<Texture>& texture) { m_AlbedoTex = texture; }

		glm::vec4& GetBaseColor() { return m_BaseColor; }
		const glm::vec4& GetBaseColor() const { return m_BaseColor; }
		void SetBaseColor(const glm::vec4& color) { m_BaseColor = color; }

		void ClearShader() { m_Shader = nullptr; }
		void ClearAlbedoTexture() { m_AlbedoTex = nullptr; }

		void Bind();
	private:
		glm::vec4 m_BaseColor = { 1,1,1,1 };
		Ref<Shader> m_Shader = nullptr;
		Ref<Texture> m_AlbedoTex = nullptr;
	};
}
