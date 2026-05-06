#include "kita_pch.h"
#include "Asset.h"
#include "AssetManager.h"
#include "core/Log.h"
#include "render/Material.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
namespace Kita{


	
	Ref<Material> MaterialAsset::CreateRuntimeMaterial() const
	{
		Ref<Material> material = CreateRef<Material>();
		ApplyToRuntimeMaterial(*material);
		return material;
	}

	void MaterialAsset::ApplyToRuntimeMaterial(Material& material) const
	{
		material.SetBaseColor(BaseColor);

		if (Asset::IsValidHandle(ShaderHandle))
		{
			auto shaderAsset = AssetManager::GetInstance().GetShaderAsset(ShaderHandle);
			if (shaderAsset && shaderAsset->GetRuntimeShader())
			{
				material.SetShader(shaderAsset->GetRuntimeShader());
			}
			else
			{
				material.ClearShader();
			}
		}
		else
		{
			material.ClearShader();
		}

		if (Asset::IsValidHandle(AlbedoTextureHandle))
		{
			auto textureAsset = AssetManager::GetInstance().GetTextureAsset(AlbedoTextureHandle);
			if (textureAsset && textureAsset->GetRuntimeTexture())
			{
				material.SetAlbedoTexture(textureAsset->GetRuntimeTexture());
			}
			else
			{
				material.ClearAlbedoTexture();
			}
		}
		else
		{
			material.ClearAlbedoTexture();
		}
	}


	void ShaderAsset::SetShaderPath(const std::filesystem::path& path)
	{
		m_ShaderPath = path;
		m_RuntimeShader = Shader::Create(m_ShaderPath.string());
	}

	void TextureAsset::SetTexturePath(const std::filesystem::path& path)
	{
		m_TexturePath = path;
		if (m_TexturePath.empty())
		{
			m_RuntimeTexture = nullptr;
			return;
		}

		TextureDescriptor desc{};
		m_RuntimeTexture = Texture::Create(desc, m_TexturePath.string());
	}

	bool MeshAsset::LoadFromFile(const std::filesystem::path& path)
	{
		m_MeshPath = path.lexically_normal();
		m_SubMeshes.clear();

		Assimp::Importer import;
		const aiScene* scene = import.ReadFile(
			m_MeshPath.string(),
			aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

		if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
		{
			KITA_CORE_ERROR("assimp import error : {0}", import.GetErrorString());
			return false;
		}

		ProcessNode(scene->mRootNode, scene);
		return true;
	}

	void MeshAsset::ProcessNode(aiNode* node, const aiScene* scene)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			ProcessMesh(mesh, scene);
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene);
		}
	}

	void MeshAsset::ProcessMesh(aiMesh* mesh, const aiScene* scene)
	{
		(void)scene;

		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		vertices.reserve(mesh->mNumVertices);

		const bool hasColors = mesh->HasVertexColors(0);
		const bool hasNormals = mesh->HasNormals();
		const bool hasTangents = mesh->HasTangentsAndBitangents();

		for (uint32_t i = 0; i < mesh->mNumVertices; i++)
		{
			Vertex vertex{};
			vertex.position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

			if (hasColors)
			{
				vertex.color = glm::vec4(
					mesh->mColors[0][i].r,
					mesh->mColors[0][i].g,
					mesh->mColors[0][i].b,
					mesh->mColors[0][i].a);
			}
			else
			{
				vertex.color = glm::vec4(1.0f);
			}

			if (hasNormals)
			{
				vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
			}
			else
			{
				vertex.normal = glm::vec3(0.0f);
			}

			if (hasTangents)
			{
				vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
				vertex.bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
			}
			else
			{
				vertex.tangent = glm::vec3(0.0f);
				vertex.bitangent = glm::vec3(0.0f);
			}

			if (mesh->mTextureCoords[0])
			{
				vertex.texcoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
			}
			else
			{
				vertex.texcoords = glm::vec2(0.0f);
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

		m_SubMeshes.push_back(Mesh::Create(vertices, indices));
	}

}
