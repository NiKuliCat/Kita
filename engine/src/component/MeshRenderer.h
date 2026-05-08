#pragma once
#include "core/Core.h"
#include "render/mesh/Mesh.h"
#include "render/VulkanMaterial.h"
#include "render/BufferLayout.h"
#include "asset/Asset.h"
namespace Kita {
	
	class MeshRenderer
	{
	public:
		MeshRenderer();
		~MeshRenderer() {}

		AssetHandle GetMeshAssetHandle() const { return m_MeshAssetHandle; }
		void SetDefaultMaterialAssetHandle(AssetHandle handle) { m_DefaultMaterialAssetHandle = handle; }
		void LoadMeshs(const std::string& filepath);
		const std::vector<Ref<Mesh>>& GetMeshs() const { return m_Meshs; }

		Ref<VulkanMaterial>& GetRuntimeMaterial(const size_t index) { return m_RuntimeMaterials[index]; }
		std::vector<Ref<VulkanMaterial>>& GetRuntimeMaterials() { return m_RuntimeMaterials; }

		AssetHandle GetMaterialAssetHandle(size_t index) const { return m_MaterialAssetHandles[index]; }
		std::vector<AssetHandle>& GetMaterialAssetHandles() { return m_MaterialAssetHandles; }
		const std::vector<AssetHandle>& GetMaterialAssetHandles() const { return m_MaterialAssetHandles; }
		void SetMaterialAssetHandle(size_t index, AssetHandle handle);
		Ref<MaterialAsset> GetMaterialAsset(size_t index) const;

		void SyncMaterial(size_t index);
		void SyncAllMaterials();


		const size_t GetSubMeshCount() const { return m_Meshs.size(); }
	private:
		void InitializeMaterialSlots(size_t slotCount);
	private:
		AssetHandle m_MeshAssetHandle = InvalidAssetHandle;
		AssetHandle m_DefaultMaterialAssetHandle = InvalidAssetHandle;
		std::vector<Ref<Mesh>> m_Meshs;

		std::vector<AssetHandle> m_MaterialAssetHandles;
		std::vector<Ref<VulkanMaterial>> m_RuntimeMaterials;
	};
}
