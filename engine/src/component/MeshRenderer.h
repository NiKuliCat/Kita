#pragma once
#include "core/Core.h"
#include "render/mesh/Mesh.h"
#include "render/Material.h"
#include <assimp/scene.h>
#include <render/Buffer.h>
#include <render/VertexArray.h>
namespace Kita {
	
	class MeshRenderer
	{
	public:
		MeshRenderer();
		~MeshRenderer() {}

		const std::string GetMeshFilePath() const { return m_MeshFilePath; }
		void LoadMeshs(const std::string& filepath);
		const std::vector<Ref<Mesh>>& GetMeshs() const { return m_Meshs; }

		Ref<Material>& GetMaterial(const size_t index) { return m_Materials[index]; }
		std::vector<Ref<Material>>& GetMaterials() { return m_Materials; }

		const size_t GetSubMeshCount() const { return m_Meshs.size(); }
	private:
		void ProcessNode(aiNode* node, const aiScene* scene);
		void ProcessMesh(aiMesh* mesh, const aiScene* scene);
	private:
		std::string m_MeshFilePath;
		std::vector<Ref<Mesh>> m_Meshs;
		std::vector<Ref<Material>>  m_Materials;
	};
}
