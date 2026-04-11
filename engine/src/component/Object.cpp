#include "kita_pch.h"
#include "Object.h"
#include "core/Log.h"

#if KITA_HAS_ASSIMP
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#endif

namespace Kita {
	void Object::LoadMeshs(const std::string& filepath)
	{
#if KITA_HAS_ASSIMP
        Assimp::Importer import;
        const aiScene* scene = import.ReadFile(filepath, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace );
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            KITA_CORE_ERROR("assimp import error : {0}", import.GetErrorString());
            return;
        }

        ProcessNode(scene->mRootNode, scene);
#else
        KITA_CORE_ERROR("assimp binary dependency is missing, cannot load mesh: {0}", filepath);
#endif
	}



#if KITA_HAS_ASSIMP
    void Object::ProcessNode(aiNode* node, const aiScene* scene)
    {
        for (unsigned int i = 0; i < node->mNumMeshes; i++)
        {
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            ProcessMesh(mesh, scene);
        }
        // Recurse into child nodes.
        for (unsigned int i = 0; i < node->mNumChildren; i++)
        {
            ProcessNode(node->mChildren[i], scene);
        }
    }

    void Object::ProcessMesh(aiMesh* mesh, const aiScene* scene)
    {
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

        bool hasColors = mesh->HasVertexColors(0);

        for (uint32_t i = 0; i < mesh->mNumVertices; i++)
        {
			Vertex vertex;

            vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            if (hasColors)
            {
                vertex.color = glm::vec4(mesh->mColors[i]->r, mesh->mColors[i]->g, mesh->mColors[i]->b, mesh->mColors[i]->a);
            }
            else
            {
				vertex.color = glm::vec4(1.0f, 1.0f,1.0f, 1.0f);
            }
			vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
			vertex.bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
            
            if (mesh->mTextureCoords[0]) 
            {
                vertex.texcoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            }
            else 
            {
                vertex.texcoords = glm::vec2(0.0f, 0.0f);
            }

		   vertices.push_back(vertex);
        }

        for (uint32_t i = 0; i < mesh->mNumFaces; i++)
        {
			auto face = mesh->mFaces[i];
            for (uint32_t j = 0; j < face.mNumIndices; j++)
            {
				indices.push_back(face.mIndices[j]);
            }
        }

		auto  meshObj = Mesh::Create(vertices, indices);

		m_Meshs.push_back(meshObj);
    }
#else
    void Object::ProcessNode(aiNode* node, const aiScene* scene)
    {
        (void)node;
        (void)scene;
    }

    void Object::ProcessMesh(aiMesh* mesh, const aiScene* scene)
    {
        (void)mesh;
        (void)scene;
    }
#endif
}
