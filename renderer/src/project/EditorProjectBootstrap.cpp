#include "renderer_pch.h"
#include "EditorProjectBootstrap.h"

#include "file/Project.h"
#include <fstream>
#include <nlohmann/json.hpp>

namespace Kita {

	std::unordered_map<std::string, std::string> EditorProjectBootstrap::m_PreLoadShaderPath{};
	std::unordered_map<std::string, std::string> EditorProjectBootstrap::m_PreLoadMaterialPath{};
	std::unordered_map<std::string, std::string> EditorProjectBootstrap::m_PreLoadMeshPath{};
	std::unordered_map<std::string, std::string> EditorProjectBootstrap::m_PreLoadTexturePath{};

	namespace
	{
		const std::filesystem::path kEditorConfigRelativePath = "packages/config/config.json";

		const std::string* FindMappedPath(
			const std::unordered_map<std::string, std::string>& pathMap,
			const std::string& name)
		{
			const auto it = pathMap.find(name);
			if (it == pathMap.end())
				return nullptr;

			return &it->second;
		}

		void ReadStringMapNode(
			const nlohmann::json& parent,
			const char* nodeName,
			std::unordered_map<std::string, std::string>& outMap)
		{
			outMap.clear();

			if (!parent.contains(nodeName))
				return;

			const nlohmann::json& node = parent.at(nodeName);
			if (!node.is_object())
			{
				KITA_CORE_WARN("EditorProjectBootstrap config node '{}' is not an object.", nodeName);
				return;
			}

			for (auto it = node.begin(); it != node.end(); ++it)
			{
				if (!it.value().is_string())
				{
					KITA_CORE_WARN("EditorProjectBootstrap config '{}.{}' is not a string.", nodeName, it.key());
					continue;
				}

				const std::filesystem::path normalizedPath = std::filesystem::path(it.value().get<std::string>()).lexically_normal();
				outMap[it.key()] = normalizedPath.generic_string();
			}
		}

		template<typename AssetT>
		Ref<AssetT> LoadTypedAssetByPath(const std::filesystem::path& path)
		{
			if (path.empty())
				return nullptr;

			auto& assetManager = AssetManager::GetInstance();
			const AssetHandle handle = assetManager.GetHandleByPath(path);
			if (!Asset::IsValidHandle(handle))
				return nullptr;

			return assetManager.GetAsset<AssetT>(handle);
		}

		void PreloadConfiguredAssets(const std::unordered_map<std::string, std::string>& pathMap)
		{
			auto& assetManager = AssetManager::GetInstance();
			for (const auto& [name, path] : pathMap)
			{
				const AssetHandle handle = assetManager.GetHandleByPath(path);
				if (!Asset::IsValidHandle(handle))
				{
					KITA_CORE_WARN("EditorProjectBootstrap missing configured asset '{}': {}", name, path);
					continue;
				}

				assetManager.LoadAsset(handle);
			}
		}
	}

	void EditorProjectBootstrap::Initialize()
	{
		const Ref<Project> project = Project::GetActive();
		if (!project)
			return;

		const std::filesystem::path configPath = project->GetAssetRootDirectory() / kEditorConfigRelativePath;
		if (!ReadConfigFile(configPath))
			return;

		PreloadConfiguredAssets(m_PreLoadShaderPath);
		PreloadConfiguredAssets(m_PreLoadMaterialPath);
		PreloadConfiguredAssets(m_PreLoadMeshPath);
		PreloadConfiguredAssets(m_PreLoadTexturePath);
	}


	Ref<ShaderAsset> EditorProjectBootstrap::GetPreLoadShader(const std::string& name)
	{
		return LoadTypedAssetByPath<ShaderAsset>(GetPreLoadShaderPath(name));
	}

	Ref<MaterialAsset> EditorProjectBootstrap::GetPreLoadMaterial(const std::string& name)
	{
		return LoadTypedAssetByPath<MaterialAsset>(GetPreLoadMaterialPath(name));
	}

	Ref<MeshAsset> EditorProjectBootstrap::GetPreLoadMesh(const std::string& name)
	{
		return LoadTypedAssetByPath<MeshAsset>(GetPreLoadMeshPath(name));
	}

	Ref<TextureAsset> EditorProjectBootstrap::GetPreLoadTexture(const std::string& name)
	{
		return LoadTypedAssetByPath<TextureAsset>(GetPreLoadTexturePath(name));
	}

	AssetHandle EditorProjectBootstrap::GetPreLoadShaderHandle(const std::string& name)
	{
		const std::filesystem::path path = GetPreLoadShaderPath(name);
		if (path.empty())
			return InvalidAssetHandle;

		return AssetManager::GetInstance().GetHandleByPath(path);
	}

	AssetHandle EditorProjectBootstrap::GetPreLoadMaterialHandle(const std::string& name)
	{
		const std::filesystem::path path = GetPreLoadMaterialPath(name);
		if (path.empty())
			return InvalidAssetHandle;

		return AssetManager::GetInstance().GetHandleByPath(path);
	}

	AssetHandle EditorProjectBootstrap::GetPreLoadMeshHandle(const std::string& name)
	{
		const std::filesystem::path path = GetPreLoadMeshPath(name);
		if (path.empty())
			return InvalidAssetHandle;

		return AssetManager::GetInstance().GetHandleByPath(path);
	}

	AssetHandle EditorProjectBootstrap::GetPreLoadTextureHandle(const std::string& name)
	{
		const std::filesystem::path path = GetPreLoadTexturePath(name);
		if (path.empty())
			return InvalidAssetHandle;

		return AssetManager::GetInstance().GetHandleByPath(path);
	}

	std::filesystem::path EditorProjectBootstrap::GetPreLoadShaderPath(const std::string& name)
	{
		const std::string* path = FindMappedPath(m_PreLoadShaderPath, name);
		return path ? std::filesystem::path(*path) : std::filesystem::path{};
	}

	std::filesystem::path EditorProjectBootstrap::GetPreLoadMaterialPath(const std::string& name)
	{
		const std::string* path = FindMappedPath(m_PreLoadMaterialPath, name);
		return path ? std::filesystem::path(*path) : std::filesystem::path{};
	}

	std::filesystem::path EditorProjectBootstrap::GetPreLoadMeshPath(const std::string& name)
	{
		const std::string* path = FindMappedPath(m_PreLoadMeshPath, name);
		return path ? std::filesystem::path(*path) : std::filesystem::path{};
	}

	std::filesystem::path EditorProjectBootstrap::GetPreLoadTexturePath(const std::string& name)
	{
		const std::string* path = FindMappedPath(m_PreLoadTexturePath, name);
		return path ? std::filesystem::path(*path) : std::filesystem::path{};
	}

	bool EditorProjectBootstrap::ReadConfigFile(const std::filesystem::path& configPath)
	{
		m_PreLoadShaderPath.clear();
		m_PreLoadMaterialPath.clear();
		m_PreLoadMeshPath.clear();
		m_PreLoadTexturePath.clear();

		if (configPath.empty())
		{
			KITA_CORE_WARN("EditorProjectBootstrap config path is empty.");
			return false;
		}

		std::ifstream in(configPath);
		if (!in.is_open())
		{
			KITA_CORE_ERROR("Failed to open editor bootstrap config: {0}", configPath.string());
			return false;
		}

		nlohmann::json root;
		try
		{
			in >> root;
		}
		catch (const std::exception& e)
		{
			KITA_CORE_ERROR("Failed to parse editor bootstrap config '{}': {}", configPath.string(), e.what());
			return false;
		}

		if (!root.is_object() || !root.contains("asset") || !root["asset"].is_object())
		{
			KITA_CORE_ERROR("Editor bootstrap config must contain an 'asset' object: {}", configPath.string());
			return false;
		}

		const nlohmann::json& assetNode = root["asset"];

		ReadStringMapNode(assetNode, "shader", m_PreLoadShaderPath);
		ReadStringMapNode(assetNode, "material", m_PreLoadMaterialPath);
		ReadStringMapNode(assetNode, "mesh", m_PreLoadMeshPath);
		ReadStringMapNode(assetNode, "texture", m_PreLoadTexturePath);

		return true;
	}

}
