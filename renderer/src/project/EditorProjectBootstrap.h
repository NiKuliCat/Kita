#pragma once

#include <Engine.h>

namespace Kita {

	class EditorProjectBootstrap
	{
	public:
		static void Initialize();

		static Ref<ShaderAsset> GetPreLoadShader(const std::string& name);
		static Ref<MaterialAsset> GetPreLoadMaterial(const std::string& name);
		static Ref<MeshAsset> GetPreLoadMesh(const std::string& name);
		static Ref<TextureAsset> GetPreLoadTexture(const std::string& name);

		static AssetHandle GetPreLoadShaderHandle(const std::string& name);
		static AssetHandle GetPreLoadMaterialHandle(const std::string& name);
		static AssetHandle GetPreLoadMeshHandle(const std::string& name);
		static AssetHandle GetPreLoadTextureHandle(const std::string& name);

		static std::filesystem::path GetPreLoadShaderPath(const std::string& name);
		static std::filesystem::path GetPreLoadMaterialPath(const std::string& name);
		static std::filesystem::path GetPreLoadMeshPath(const std::string& name);
		static std::filesystem::path GetPreLoadTexturePath(const std::string& name);

	private:
		static bool ReadConfigFile(const std::filesystem::path& configPath);

		static std::unordered_map<std::string, std::string>  m_PreLoadShaderPath;
		static std::unordered_map<std::string, std::string>  m_PreLoadMaterialPath;
		static std::unordered_map<std::string, std::string>  m_PreLoadMeshPath;
		static std::unordered_map<std::string, std::string>  m_PreLoadTexturePath;
	};

}
