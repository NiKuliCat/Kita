#pragma once
#include "core/Core.h"
#include "render/mesh/Mesh.h"
#include "render/Shader.h"
#include <assimp/scene.h>
namespace Kita {
	
	class MeshRenderer
	{
	public:
		MeshRenderer() = default;
		~MeshRenderer() {}


		void LoadMeshs(const std::string& filepath);

		const std::vector<Ref<Mesh>>& GetMeshs() const { return m_Meshs; }
	private:
		void ProcessNode(aiNode* node, const aiScene* scene);
		void ProcessMesh(aiMesh* mesh, const aiScene* scene);

	private:
		std::vector<Ref<Mesh>> m_Meshs;
	};
}
