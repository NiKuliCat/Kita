#include "kita_pch.h"

#include "Mesh.h"
#include "render/VulkanContext.h"
#include "render/VulkanGeometry.h"
#include "core/Log.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
namespace Kita
{
	namespace
	{
		void ProcessAssimpMesh(aiMesh* mesh, std::vector<Ref<Mesh>>& outputMeshes)
		{
			if (!mesh)
				return;

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

				if (mesh->mTextureCoords[0])
				{
					vertex.texcoords = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
				}
				else
				{
					vertex.texcoords = glm::vec2(0.0f);
				}

				if (hasNormals)
				{
					vertex.normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
				}
				else
				{
					vertex.normal = glm::vec3(0.0f, 0.0f, 1.0f);
				}

				if (hasTangents)
				{
					vertex.tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
					vertex.bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
				}
				else
				{
					vertex.tangent = glm::vec3(1.0f, 0.0f, 0.0f);
					vertex.bitangent = glm::vec3(0.0f, 1.0f, 0.0f);
				}

				vertices.push_back(vertex);
			}

			for (uint32_t i = 0; i < mesh->mNumFaces; i++)
			{
				const aiFace& face = mesh->mFaces[i];
				for (uint32_t j = 0; j < face.mNumIndices; j++)
				{
					indices.push_back(face.mIndices[j]);
				}
			}

			outputMeshes.push_back(Mesh::Create(vertices, indices));
		}

		void ProcessAssimpNode(aiNode* node, const aiScene* scene, std::vector<Ref<Mesh>>& outputMeshes)
		{
			if (!node || !scene)
				return;

			for (unsigned int i = 0; i < node->mNumMeshes; i++)
			{
				aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
				ProcessAssimpMesh(mesh, outputMeshes);
			}

			for (unsigned int i = 0; i < node->mNumChildren; i++)
			{
				ProcessAssimpNode(node->mChildren[i], scene, outputMeshes);
			}
		}
	}

    Mesh::Mesh(const std::vector<Vertex>& vertex, const std::vector<uint32_t> indices)
		:m_Vertices(vertex), m_Indices(indices)
    {
    }
    Mesh::~Mesh()
    {
    }

	std::vector<Ref<Mesh>> Mesh::LoadMeshesFromFile(const std::filesystem::path& path)
	{
		std::vector<Ref<Mesh>> meshes;

		Assimp::Importer importer;
		const aiScene* scene = importer.ReadFile(
			path.string(),
			aiProcess_Triangulate |
			aiProcess_FlipUVs |
			aiProcess_CalcTangentSpace |
			aiProcess_GenSmoothNormals |
			aiProcess_JoinIdenticalVertices);

		if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
		{
			KITA_CORE_ERROR("Failed to import mesh '{0}': {1}", path.string(), importer.GetErrorString());
			return meshes;
		}

		ProcessAssimpNode(scene->mRootNode, scene, meshes);

		if (meshes.empty())
		{
			KITA_CORE_WARN("Imported mesh '{0}' but no submeshes were produced.", path.string());
		}

		return meshes;
	}

	void Mesh::CreateVulkanGeometry(VulkanContext& context)
	{
		BufferLayout meshLayout = {
		   {ShaderDataType::Float3, "position"},
		   {ShaderDataType::Float4, "color"},
		   {ShaderDataType::Float2, "texcoords"},
		   {ShaderDataType::Float3, "normal"},
		   {ShaderDataType::Float3, "tangent"},
		   {ShaderDataType::Float3, "bitangent"}
		};

		VulkanGeometry::CreateInfo createInfo{};
		createInfo.Name = "MeshGeometry";
		createInfo.VertexData = m_Vertices.data();
		createInfo.VertexDataSize = static_cast<uint32_t>(sizeof(Vertex) * m_Vertices.size());
		createInfo.VertexCount = static_cast<uint32_t>(m_Vertices.size());
		createInfo.VertexLayout = meshLayout;
		createInfo.IndexData = m_Indices.empty() ? nullptr : m_Indices.data();
		createInfo.IndexCount = static_cast<uint32_t>(m_Indices.size());
		createInfo.Dynamic = false;

		m_VulkanGeometry = CreateUnique<VulkanGeometry>(context, createInfo);
	}
}
