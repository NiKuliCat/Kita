#pragma once

#include "core/Core.h"
#include "render/mesh/Mesh.h"


#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace Kita {


	class Object {
	public:
		Object(const std::string& name)
			:m_Name(name) {}
		~Object(){}


		void LoadMeshs(const std::string& filepath);

		const std::vector<Ref<Mesh>>& GetMeshs() const  { return m_Meshs; }
	private:
		void ProcessNode(aiNode* node, const aiScene* scene);
		void ProcessMesh(aiMesh* mesh, const aiScene* scene);
		
	private:
		std::string m_Name;
		std::vector<Ref<Mesh>> m_Meshs;
	};
}