#include "kita_pch.h"
#include "Asset.h"
#include "AssetManager.h"
#include "core/Log.h"
#include "render/VulkanMaterial.h"
#include "render/ShaderCompiler.h"
#include "render/VulkanTextureLoader.h"
#include "core/Application.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
namespace Kita{
	Ref<VulkanMaterial> MaterialAsset::CreateRuntimeMaterial() const
	{
		Ref<VulkanMaterial> material = CreateRef<VulkanMaterial>();
		ApplyToRuntimeMaterial(*material);
		return material;
	}

	void MaterialAsset::ApplyToRuntimeMaterial(VulkanMaterial& material) const
	{
		material.SetBaseColor(BaseColor);
	}


	void ShaderAsset::SetShaderPath(const std::filesystem::path& path)
	{
		m_ShaderPath = path.lexically_normal();
		Compile();
	}

	bool ShaderAsset::Compile()
	{
		ShaderCompiler compiler;

		ShaderCompiler::CompileRequest vs{};
		vs.SourcePath = m_ShaderPath;
		vs.ModuleName = m_ShaderPath.stem().string() + "_vs";
		vs.EntryPointName = "VSMain";
		vs.ShaderStage = ShaderCompiler::Stage::Vertex;

		ShaderCompiler::CompileRequest fs{};
		fs.SourcePath = m_ShaderPath;
		fs.ModuleName = m_ShaderPath.stem().string() + "_fs";
		fs.EntryPointName = "PSMain";
		fs.ShaderStage = ShaderCompiler::Stage::Fragment;

		auto vsResult = compiler.CompileToSpirv(vs);
		auto fsResult = compiler.CompileToSpirv(fs);

		if (!vsResult.Success || !fsResult.Success)
			return false;

		m_VertexStage.Spirv = std::move(vsResult.Spirv);
		m_FragmentStage.Spirv = std::move(fsResult.Spirv);
		m_VertexStage.EntryPoint = "main";
		m_FragmentStage.EntryPoint = "main";
		return true;
	}

	void TextureAsset::SetTexturePath(const std::filesystem::path& path)
	{
		m_TexturePath = path.lexically_normal();
		if (m_TexturePath.empty())
		{
			m_RuntimeTexture = nullptr;
			return;
		}

		VulkanContext& context = Application::Get().GetVulkanContext();
		m_RuntimeTexture = VulkanTextureLoader::LoadTexture2D(context, m_TexturePath);
		if (!m_RuntimeTexture)
		{
			KITA_CORE_WARN("Failed to create runtime VulkanTexture for asset: {}", m_TexturePath.string());
		}
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
