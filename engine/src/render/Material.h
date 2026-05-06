#pragma once

#include "Shader.h"
#include "Texture.h"
namespace Kita {


	class Material
	{
	public:
		Material() = default;
		Material(const std::string& shader_path, const std::string& tex_path);

		Ref<Shader>& GetShader() { return m_Shader; }
		const Ref<Shader>& GetShader() const { return m_Shader; }
		void SetShader(const Ref<Shader>& shader);
		void SetShader(const std::string& path);
		void ClearShader();

		const Ref<Texture>& GetAlbedoTexture() const { return m_AlbedoTex; }
		void SetAlbedoTexture(const Ref<Texture>& texture);
		void SetAlbedoTexture(const std::string& path);
		void ClearAlbedoTexture();

		glm::vec4& GetBaseColor() { return m_BaseColor; }
		const glm::vec4& GetBaseColor() const { return m_BaseColor; }
		void SetBaseColor(const glm::vec4& color) { m_BaseColor = color; }

		void Bind();
	private:
		glm::vec4 m_BaseColor = { 1,1,1,1 };
		Ref<Shader> m_Shader = nullptr;
		Ref<Texture> m_AlbedoTex = nullptr;
	};
}
