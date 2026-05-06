#pragma once
#include "core/Core.h"
#include "render/mesh/Mesh.h"
#include "render/Material.h"
#include <assimp/scene.h>
#include <render/Buffer.h>
#include <render/VertexArray.h>
#include <asset/Asset.h>
namespace Kita {
	
	class MeshRenderer
	{
	public:
		MeshRenderer();
		~MeshRenderer() {}

		const std::string GetMeshFilePath() const { return m_MeshFilePath; }
		void LoadMeshs(const std::string& filepath);
		const std::vector<Ref<Mesh>>& GetMeshs() const { return m_Meshs; }

		Ref<Material>& GetRuntimeMaterial(const size_t index) { return m_RuntimeMaterials[index]; }
		std::vector<Ref<Material>>& GetRuntimeMaterials() { return m_RuntimeMaterials; }

		Ref<MaterialAsset>& GetMaterialAsset(size_t index) { return m_MaterialAssets[index]; }
		std::vector<Ref<MaterialAsset>>& GetMaterialAssets() { return m_MaterialAssets; }

		void SyncMaterial(size_t index);
		void SyncAllMaterials();


		const size_t GetSubMeshCount() const { return m_Meshs.size(); }
	private:
		void ProcessNode(aiNode* node, const aiScene* scene);
		void ProcessMesh(aiMesh* mesh, const aiScene* scene);
	private:
		std::string m_MeshFilePath;
		std::vector<Ref<Mesh>> m_Meshs;

		std::vector<Ref<MaterialAsset>> m_MaterialAssets;
		std::vector<Ref<Material>> m_RuntimeMaterials;
	};
}
