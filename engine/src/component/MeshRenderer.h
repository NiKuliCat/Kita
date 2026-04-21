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


		void LoadMeshs(const std::string& filepath);

		

		const std::vector<Ref<Mesh>>& GetMeshs() const { return m_Meshs; }

		Ref<Material>& GetMaterial() { return m_Material; }
	private:
		void ProcessNode(aiNode* node, const aiScene* scene);
		void ProcessMesh(aiMesh* mesh, const aiScene* scene);
		void InitBuffer();
	private:
		std::vector<Ref<Mesh>> m_Meshs;
		Ref<Material>  m_Material = nullptr;
	};
}
