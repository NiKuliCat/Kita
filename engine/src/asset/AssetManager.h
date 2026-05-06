#pragma once
#include "asset/Asset.h"
#include "core/Core.h"

namespace Kita {

	class AssetManager
	{
	public:
		static AssetManager& GetInstance();
		AssetManager(const AssetManager&) = delete;
		AssetManager& operator=(const AssetManager&) = delete;

		void ScanAssets(const std::filesystem::path& assetRoot);

		AssetHandle ImportAsset(const std::filesystem::path& path, const AssetType type);
		AssetHandle ImportAsset(const std::filesystem::path& path);
		bool RegisterAsset(const AssetMetadata& metadata);

		bool HasHandle(AssetHandle handle) const;
		bool HasPath(const std::filesystem::path& path) const;

		AssetHandle GetHandleByPath(const std::filesystem::path& path) const;
		const AssetMetadata* GetMetadata(AssetHandle handle) const;
		std::vector<AssetMetadata> GetAssetsByType(AssetType type) const;

		Ref<Asset> LoadAsset(AssetHandle handle);

		template<typename T>
		Ref<T> GetAsset(AssetHandle handle)
		{
			return std::dynamic_pointer_cast<T>(LoadAsset(handle));
		}

		Ref<ShaderAsset> GetShaderAsset(AssetHandle handle);
		Ref<TextureAsset> GetTextureAsset(AssetHandle handle);
		Ref<MaterialAsset> GetMaterialAsset(AssetHandle handle);
		Ref<MeshAsset> GetMeshAsset(AssetHandle handle);

		const std::filesystem::path& GetAssetRoot() const { return m_AssetRoot; }

		void Clear();

	private:
		AssetManager() = default;

		std::filesystem::path NormalizePath(const std::filesystem::path& path) const;
		std::filesystem::path MakeRelativeToAssetRoot(const std::filesystem::path& path) const;
		std::filesystem::path ResolveAssetPath(const std::filesystem::path& path) const;
		std::filesystem::path GetMetaPath(const std::filesystem::path& assetPath) const;

		bool IsAssetFile(const std::filesystem::path& path) const;
		bool IsMetaFile(const std::filesystem::path& path) const;
		AssetType GetAssetTypeFromPath(const std::filesystem::path& path) const;

		bool ReadMetaFile(const std::filesystem::path& metaPath, AssetMetadata& outMetadata) const;
		bool WriteMetaFile(const std::filesystem::path& metaPath, const AssetMetadata& metadata) const;

	private:
		std::filesystem::path m_AssetRoot;
		std::unordered_map<AssetHandle, AssetMetadata> m_MetadataRegistry;
		std::unordered_map<std::string, AssetHandle> m_PathToHandle;
		std::unordered_map<AssetHandle, Ref<Asset>> m_LoadedAssets;
	};
}
