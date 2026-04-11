#pragma once

#include "core/Core.h"
#include "render/mesh/Mesh.h"


#ifndef KITA_HAS_ASSIMP
#define KITA_HAS_ASSIMP 1
#endif

#if KITA_HAS_ASSIMP
#include <assimp/scene.h>
#else
struct aiScene;
struct aiNode;
struct aiMesh;
#endif

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
