#include "kita_pch.h"
#include "Asset.h"
#include "AssetFactory.h"
#include "AssetManager.h"
#include "core/Log.h"
#include "serialize/JsonUtils.h"
namespace Kita {

	namespace
	{
		bool ShouldScanAsAssetFile(const std::filesystem::path& path)
		{
			std::string extension = path.extension().string();
			std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

			if (extension == ".meta")
			{
				return false;
			}

			return extension == ".mat"
				|| extension == ".glsl"
				|| extension == ".vert"
				|| extension == ".frag"
				|| extension == ".png"
				|| extension == ".jpg"
				|| extension == ".jpeg"
				|| extension == ".tga"
				|| extension == ".bmp"
				|| extension == ".fbx"
				|| extension == ".obj"
				|| extension == ".dae"
				|| extension == ".gltf"
				|| extension == ".glb";
		}

		void ScanAssetDirectoryRecursive(
			AssetManager& assetManager,
			const std::filesystem::path& directory)
		{
			if (!std::filesystem::exists(directory) || !std::filesystem::is_directory(directory))
			{
				return;
			}

			for (const auto& entry : std::filesystem::recursive_directory_iterator(directory))
			{
				if (!entry.is_regular_file())
				{
					continue;
				}

				const auto& filePath = entry.path();
				if (!ShouldScanAsAssetFile(filePath))
				{
					continue;
				}

				if (!assetManager.ImportAsset(filePath))
				{
					continue;
				}
			}
		}
	}

	AssetManager& AssetManager::GetInstance()
	{
		static AssetManager instance;
		return instance;
	}

	void AssetManager::ScanAssets(const std::filesystem::path& assetRoot)
	{
		m_AssetRoot = NormalizePath(assetRoot);
		if (m_AssetRoot.empty())
		{
			KITA_CORE_WARN("ScanAssets failed, asset root is empty.");
			return;
		}

		if (!std::filesystem::exists(m_AssetRoot))
		{
			KITA_CORE_WARN("ScanAssets failed, asset root does not exist: {}", m_AssetRoot.string());
			return;
		}

		Clear();

		const std::filesystem::path contentDirectory = m_AssetRoot / "content";
		const std::filesystem::path renderPackageDirectory = m_AssetRoot / "packages" / "render";
		bool scannedAnyDirectory = false;

		if (std::filesystem::exists(contentDirectory))
		{
			ScanAssetDirectoryRecursive(*this, contentDirectory);
			scannedAnyDirectory = true;
		}

		if (std::filesystem::exists(renderPackageDirectory))
		{
			ScanAssetDirectoryRecursive(*this, renderPackageDirectory);
			scannedAnyDirectory = true;
		}

		if (!scannedAnyDirectory)
		{
			ScanAssetDirectoryRecursive(*this, m_AssetRoot);
		}
	}

	AssetHandle AssetManager::ImportAsset(const std::filesystem::path& path, AssetType type)
	{
		std::filesystem::path normalizedPath = NormalizePath(path);
		if (type == AssetType::None)
		{
			KITA_CORE_WARN("ImportAsset failed, invalid asset type: {}", normalizedPath.string());
			return InvalidAssetHandle;
		}

		std::filesystem::path relativePath = MakeRelativeToAssetRoot(normalizedPath);
		std::string key = relativePath.generic_string();

		if (key.empty())
		{
			KITA_CORE_WARN("ImportAsset failed, empty path.");
			return InvalidAssetHandle;
		}

		auto pathIt = m_PathToHandle.find(key);
		if (pathIt != m_PathToHandle.end())
		{
			return pathIt->second;
		}

		AssetMetadata metadata;
		std::filesystem::path metaPath = GetMetaPath(normalizedPath);
		if (std::filesystem::exists(metaPath))
		{
			if (!ReadMetaFile(metaPath, metadata))
			{
				KITA_CORE_WARN("ImportAsset failed to read meta file: {}", metaPath.string());
				return InvalidAssetHandle;
			}
		}
		else
		{
			metadata.handle = Asset::GenerateAssetHandle();
			metadata.type = type;
			metadata.relativePath = relativePath;

			if (!WriteMetaFile(metaPath, metadata))
			{
				KITA_CORE_WARN("ImportAsset failed to write meta file: {}", metaPath.string());
				return InvalidAssetHandle;
			}
		}

		metadata.type = type;
		metadata.relativePath = relativePath;

		if (!RegisterAsset(metadata))
		{
			return InvalidAssetHandle;
		}

		return metadata.handle;
	}

	AssetHandle AssetManager::ImportAsset(const std::filesystem::path& path)
	{
		std::filesystem::path normalizedPath = NormalizePath(path);
		if (!IsAssetFile(normalizedPath))
		{
			KITA_CORE_WARN("ImportAsset skipped unsupported file: {}", normalizedPath.string());
			return InvalidAssetHandle;
		}

		return ImportAsset(normalizedPath, GetAssetTypeFromPath(normalizedPath));
	}

	bool AssetManager::RegisterAsset(const AssetMetadata& metadata)
	{
		if (!Asset::IsValidHandle(metadata.handle))
		{
			KITA_CORE_WARN("RegisterAsset failed, invalid handle.");
			return false;
		}

		if (metadata.type == AssetType::None)
		{
			KITA_CORE_WARN("RegisterAsset failed, invalid asset type.");
			return false;
		}

		std::filesystem::path relativePath = NormalizePath(metadata.relativePath);
		std::string key = relativePath.generic_string();
		if (key.empty())
		{
			KITA_CORE_WARN("RegisterAsset failed, empty path.");
			return false;
		}

		auto existingHandleIt = m_MetadataRegistry.find(metadata.handle);
		if (existingHandleIt != m_MetadataRegistry.end())
		{
			existingHandleIt->second.type = metadata.type;
			existingHandleIt->second.relativePath = relativePath;
		}
		else
		{
			AssetMetadata storedMetadata = metadata;
			storedMetadata.relativePath = relativePath;
			m_MetadataRegistry[metadata.handle] = storedMetadata;
		}

		m_PathToHandle[key] = metadata.handle;
		return true;
	}

	bool AssetManager::HasHandle(AssetHandle handle) const
	{
		return m_MetadataRegistry.find(handle) != m_MetadataRegistry.end();
	}

	bool AssetManager::HasPath(const std::filesystem::path& path) const
	{
		std::string key = MakeRelativeToAssetRoot(path).generic_string();
		return m_PathToHandle.find(key) != m_PathToHandle.end();
	}

	AssetHandle AssetManager::GetHandleByPath(const std::filesystem::path& path) const
	{
		std::string key = MakeRelativeToAssetRoot(path).generic_string();
		auto it = m_PathToHandle.find(key);
		if (it == m_PathToHandle.end())
		{
			return InvalidAssetHandle;
		}

		return it->second;
	}

	const AssetMetadata* AssetManager::GetMetadata(AssetHandle handle) const
	{
		auto it = m_MetadataRegistry.find(handle);
		if (it == m_MetadataRegistry.end())
		{
			return nullptr;
		}

		return &it->second;
	}

	std::vector<AssetMetadata> AssetManager::GetAssetsByType(AssetType type) const
	{
		std::vector<AssetMetadata> result;
		for (const auto& [handle, metadata] : m_MetadataRegistry)
		{
			if (metadata.type == type)
			{
				result.push_back(metadata);
			}
		}

		std::sort(result.begin(), result.end(),
			[](const AssetMetadata& left, const AssetMetadata& right)
			{
				return left.relativePath.generic_string() < right.relativePath.generic_string();
			});

		return result;
	}

	Ref<Asset> AssetManager::LoadAsset(AssetHandle handle)
	{
		auto loadedIt = m_LoadedAssets.find(handle);
		if (loadedIt != m_LoadedAssets.end())
		{
			return loadedIt->second;
		}

		auto metadataIt = m_MetadataRegistry.find(handle);
		if (metadataIt == m_MetadataRegistry.end())
		{
			KITA_CORE_WARN("LoadAsset failed, invalid handle: {}", handle);
			return nullptr;
		}

		const std::filesystem::path assetPath = ResolveAssetPath(metadataIt->second.relativePath);
		Ref<Asset> asset = AssetFactory::CreateFromMetadata(metadataIt->second, assetPath);
		if (asset)
		{
			m_LoadedAssets[handle] = asset;
		}

		return asset;
	}

	Ref<ShaderAsset> AssetManager::GetShaderAsset(AssetHandle handle)
	{
		return GetAsset<ShaderAsset>(handle);
	}

	Ref<TextureAsset> AssetManager::GetTextureAsset(AssetHandle handle)
	{
		return GetAsset<TextureAsset>(handle);
	}

	Ref<MaterialAsset> AssetManager::GetMaterialAsset(AssetHandle handle)
	{
		return GetAsset<MaterialAsset>(handle);
	}

	Ref<MeshAsset> AssetManager::GetMeshAsset(AssetHandle handle)
	{
		return GetAsset<MeshAsset>(handle);
	}

	void AssetManager::Clear()
	{
		m_MetadataRegistry.clear();
		m_PathToHandle.clear();
		m_LoadedAssets.clear();
	}

	std::filesystem::path AssetManager::NormalizePath(const std::filesystem::path& path) const
	{
		if (path.empty())
		{
			return {};
		}

		return path.lexically_normal();
	}

	std::filesystem::path AssetManager::MakeRelativeToAssetRoot(const std::filesystem::path& path) const
	{
		std::filesystem::path normalizedPath = NormalizePath(path);
		if (m_AssetRoot.empty())
		{
			return normalizedPath;
		}

		std::error_code ec;
		std::filesystem::path relativePath = std::filesystem::relative(normalizedPath, m_AssetRoot, ec);
		if (ec)
		{
			return normalizedPath;
		}

		return NormalizePath(relativePath);
	}

	std::filesystem::path AssetManager::ResolveAssetPath(const std::filesystem::path& path) const
	{
		std::filesystem::path normalizedPath = NormalizePath(path);
		if (normalizedPath.is_absolute() || m_AssetRoot.empty())
		{
			return normalizedPath;
		}

		return NormalizePath(m_AssetRoot / normalizedPath);
	}

	std::filesystem::path AssetManager::GetMetaPath(const std::filesystem::path& assetPath) const
	{
		return NormalizePath(assetPath.string() + ".meta");
	}

	bool AssetManager::IsAssetFile(const std::filesystem::path& path) const
	{
		if (IsMetaFile(path))
		{
			return false;
		}

		return GetAssetTypeFromPath(path) != AssetType::None;
	}

	bool AssetManager::IsMetaFile(const std::filesystem::path& path) const
	{
		return NormalizePath(path).extension() == ".meta";
	}

	AssetType AssetManager::GetAssetTypeFromPath(const std::filesystem::path& path) const
	{
		std::string ext = NormalizePath(path).extension().string();
		std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

		if (ext == ".mat")
		{
			return AssetType::Material;
		}

		if (ext == ".glsl" || ext == ".vert" || ext == ".frag")
		{
			return AssetType::Shader;
		}

		if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp")
		{
			return AssetType::Texture;
		}

		if (ext == ".fbx" || ext == ".obj" || ext == ".dae" || ext == ".gltf" || ext == ".glb")
		{
			return AssetType::Mesh;
		}

		return AssetType::None;
	}

	bool AssetManager::ReadMetaFile(const std::filesystem::path& metaPath, AssetMetadata& outMetadata) const
	{
		std::ifstream input(metaPath);
		if (!input.is_open())
		{
			return false;
		}

		json root;
		input >> root;

		if (!root.is_object())
		{
			return false;
		}

		if (!root.contains("handle") || !root.contains("type"))
		{
			return false;
		}

		outMetadata.handle = root["handle"].get<uint64_t>();
		outMetadata.type = static_cast<AssetType>(root["type"].get<uint32_t>());
		return outMetadata.type != AssetType::None && Asset::IsValidHandle(outMetadata.handle);
	}

	bool AssetManager::WriteMetaFile(const std::filesystem::path& metaPath, const AssetMetadata& metadata) const
	{
		std::ofstream output(metaPath);
		if (!output.is_open())
		{
			return false;
		}

		json root;
		root["handle"] = metadata.handle;
		root["type"] = static_cast<uint32_t>(metadata.type);
		root["version"] = 1;

		output << root.dump(4);
		return true;
	}
}
