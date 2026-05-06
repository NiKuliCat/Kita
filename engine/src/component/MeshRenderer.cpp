#include "kita_pch.h"
#include "MeshRenderer.h"
#include <filesystem>
#include "core/Log.h"
#include "asset/AssetManager.h"

namespace
{
	std::filesystem::path ResolveMeshAssetPath(
		const std::filesystem::path& path,
		const std::filesystem::path& assetRoot)
	{
		if (path.empty())
		{
			return {};
		}

		if (path.is_absolute())
		{
			return path.lexically_normal();
		}

		const std::filesystem::path fromWorkingDirectory = (std::filesystem::current_path() / path).lexically_normal();
		if (std::filesystem::exists(fromWorkingDirectory))
		{
			return fromWorkingDirectory;
		}

		if (!assetRoot.empty())
		{
			const std::filesystem::path fromAssetRoot = (assetRoot / path).lexically_normal();
			if (std::filesystem::exists(fromAssetRoot))
			{
				return fromAssetRoot;
			}
		}

		return fromWorkingDirectory;
	}
}

namespace Kita {
    MeshRenderer::MeshRenderer()
    {
    }
    void MeshRenderer::LoadMeshs(const std::string& filepath)
    {
        m_MeshFilePath.clear();
        m_MeshAssetHandle = InvalidAssetHandle;
        m_Meshs.clear();
        m_MaterialAssetHandles.clear();
        m_RuntimeMaterials.clear();

        auto& assetManager = AssetManager::GetInstance();
        const std::filesystem::path assetPath = ResolveMeshAssetPath(filepath, assetManager.GetAssetRoot());
        m_MeshAssetHandle = assetManager.ImportAsset(assetPath);

        if (!Asset::IsValidHandle(m_MeshAssetHandle))
        {
            KITA_CORE_WARN("Failed to import mesh asset: {}", assetPath.string());
            return;
        }

        if (const AssetMetadata* metadata = assetManager.GetMetadata(m_MeshAssetHandle))
        {
            m_MeshFilePath = metadata->relativePath.generic_string();
        }
        else
        {
            m_MeshFilePath = assetPath.generic_string();
        }

        Ref<MeshAsset> meshAsset = assetManager.GetMeshAsset(m_MeshAssetHandle);
        if (!meshAsset)
        {
            KITA_CORE_WARN("Failed to load mesh asset: {}", assetPath.string());
            m_MeshAssetHandle = InvalidAssetHandle;
            return;
        }

        m_Meshs = meshAsset->GetSubMeshes();
        InitializeMaterialSlots(m_Meshs.size());
    }

    void MeshRenderer::SyncMaterial(size_t index)
    {
        if (index >= m_MaterialAssetHandles.size())
            return;

        if (index >= m_RuntimeMaterials.size())
            m_RuntimeMaterials.resize(m_MaterialAssetHandles.size());

        if (!m_RuntimeMaterials[index])
            m_RuntimeMaterials[index] = CreateRef<Material>();

        Ref<MaterialAsset> materialAsset = GetMaterialAsset(index);
        if (!materialAsset)
        {
            materialAsset = CreateRef<MaterialAsset>();
            materialAsset->BaseColor = glm::vec4(1.0f);
        }

        materialAsset->ApplyToRuntimeMaterial(*m_RuntimeMaterials[index]);
    }

    void MeshRenderer::SyncAllMaterials()
    {
        for (size_t i = 0; i < m_MaterialAssetHandles.size(); i++)
        {
            SyncMaterial(i);
        }
    }

    void MeshRenderer::InitializeMaterialSlots(size_t slotCount)
    {
        m_MaterialAssetHandles.clear();
        m_RuntimeMaterials.clear();
        m_MaterialAssetHandles.reserve(slotCount);
        m_RuntimeMaterials.reserve(slotCount);

        for (size_t i = 0; i < slotCount; ++i)
        {
            m_MaterialAssetHandles.push_back(m_DefaultMaterialAssetHandle);
            m_RuntimeMaterials.push_back(CreateRef<Material>());
            SyncMaterial(i);
        }
    }

    void MeshRenderer::SetMaterialAssetHandle(size_t index, AssetHandle handle)
    {
        if (index >= m_MaterialAssetHandles.size())
            return;

        m_MaterialAssetHandles[index] = handle;
    }

    Ref<MaterialAsset> MeshRenderer::GetMaterialAsset(size_t index) const
    {
        if (index >= m_MaterialAssetHandles.size())
            return nullptr;

        const AssetHandle handle = m_MaterialAssetHandles[index];
        if (!Asset::IsValidHandle(handle))
            return nullptr;

        return AssetManager::GetInstance().GetMaterialAsset(handle);
    }

}
