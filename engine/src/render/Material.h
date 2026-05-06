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
		void SetShader(const std::string& path);

		void SetAlbedoTexture(const std::string& path);

		const std::string& GetShaderFilePath() const { return m_ShaderFilePath; }
		const std::string& GetAlbedoTexturePath() const { return m_AlbedoTexPath; }

		glm::vec4& GetBaseColor() { return m_BaseColor; }
		const glm::vec4& GetBaseColor() const { return m_BaseColor; }
		void SetBaseColor(const glm::vec4& color) { m_BaseColor = color; }

		void Bind();
	private:

		std::string m_ShaderFilePath;
		std::string m_AlbedoTexPath;
		glm::vec4 m_BaseColor = { 1,1,1,1 };
		Ref<Shader> m_Shader = nullptr;
		Ref<Texture> m_AlbedoTex = nullptr;
	};
}