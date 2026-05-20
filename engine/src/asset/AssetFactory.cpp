#include "kita_pch.h"
#include "AssetFactory.h"
#include "AssetManager.h"
#include "core/Log.h"
#include "serialize/MaterialSerializer.h"
#include "render/ShaderCompiler.h"
#include "render/VulkanTextureLoader.h"

#include "third_party/stb_image/stb_image.h"
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace Kita{
	Ref<Asset> AssetFactory::CreateFromMetadata(const AssetMetadata& metadata, const std::filesystem::path& assetPath)
	{
		switch (metadata.type)
		{
		case AssetType::Material:
		{
			Ref<MaterialAsset> materialAsset = CreateRef<MaterialAsset>();
			materialAsset->m_Handle = metadata.handle;

			if (!MaterialSerializer::Deserialize(assetPath, *materialAsset))
			{
				KITA_CORE_WARN("AssetFactory: failed to deserialize material asset: {}", assetPath.string());
				return nullptr;
			}

			return materialAsset;
		}

		case AssetType::Shader:
		{
			Ref<ShaderAsset> shaderAsset = CreateRef<ShaderAsset>();
			shaderAsset->m_Handle = metadata.handle;
			shaderAsset->SourcePath = assetPath.lexically_normal();

			if (!AssetBuilder::CompilerShader(*shaderAsset))
			{
				KITA_CORE_WARN("AssetFactory: failed to compile shader asset: {}", assetPath.string());
				return nullptr;
			}

			return shaderAsset;
		}

		case AssetType::Texture:
		{
			Ref<TextureAsset> textureAsset = CreateRef<TextureAsset>();
			textureAsset->m_Handle = metadata.handle;
			textureAsset->SourcePath = assetPath.lexically_normal();
			AssetManager::GetInstance().GetTextureImportSettings(metadata.handle, textureAsset->ImportSettings);

			if (!AssetBuilder::LoadPixel(*textureAsset))
			{
				KITA_CORE_WARN("AssetFactory: failed to load texture asset: {}", assetPath.string());
				return nullptr;
			}

			return textureAsset;
		}

		case AssetType::Mesh:
		{
			Ref<MeshAsset> meshAsset = CreateRef<MeshAsset>();
			meshAsset->m_Handle = metadata.handle;
			meshAsset->SourcePath = assetPath.lexically_normal();

			if (!AssetBuilder::LoadMeshData_FBX(*meshAsset))
			{
				KITA_CORE_WARN("AssetFactory: failed to load mesh asset: {}", assetPath.string());
				return nullptr;
			}

			return meshAsset;
		}

		case AssetType::None:
		default:
			KITA_CORE_WARN("AssetFactory: unsupported asset type for path: {}", assetPath.string());
			return nullptr;
		}
	}
	bool AssetBuilder::CompilerShader(ShaderAsset& shaderAsset)
	{
		ShaderCompiler compiler;
		const std::filesystem::path shaderDirectory = shaderAsset.SourcePath.parent_path();
		KITA_CORE_INFO("shader include dir : {0}", shaderDirectory.string());
		ShaderCompiler::CompileRequest vs{};
		vs.SourcePath = shaderAsset.SourcePath;
		vs.ModuleName = shaderAsset.SourcePath.stem().string() + "_vs";
		vs.EntryPointName = "VSMain";
		vs.ShaderStage = ShaderCompiler::Stage::Vertex;
		if (!shaderDirectory.empty())
		{
			vs.IncludeDirs.push_back(shaderDirectory);
		}

		ShaderCompiler::CompileRequest fs{};
		fs.SourcePath = shaderAsset.SourcePath;
		fs.ModuleName = shaderAsset.SourcePath.stem().string() + "_fs";
		fs.EntryPointName = "PSMain";
		fs.ShaderStage = ShaderCompiler::Stage::Fragment;
		if (!shaderDirectory.empty())
		{
			fs.IncludeDirs.push_back(shaderDirectory);
		}

		auto vsResult = compiler.CompileToSpirv(vs);
		auto fsResult = compiler.CompileToSpirv(fs);

		if (!vsResult.Success || !fsResult.Success)
			return false;

		ShaderStageBinary  vertex;
		ShaderStageBinary  frag;

		vertex.Spirv = std::move(vsResult.Spirv);
		vertex.EntryPoint = "main";

		frag.Spirv = std::move(fsResult.Spirv);
		frag.EntryPoint = "main";

		shaderAsset.VertexStage = vertex;
		shaderAsset.FragmentStage = frag;

		return true;
	}
	bool AssetBuilder::LoadPixel(TextureAsset& textureAsset)
	{
		const std::filesystem::path normalizedPath = textureAsset.SourcePath.lexically_normal();

		if (!std::filesystem::exists(normalizedPath))
		{
			KITA_CORE_ERROR("VulkanTextureLoader: texture file does not exist: {0}", normalizedPath.string());
			return false;
		}

		int width = 0;
		int height = 0;
		int channels = 0;

		stbi_set_flip_vertically_on_load(0);

		stbi_uc* pixels = stbi_load(normalizedPath.string().c_str(), &width, &height ,&channels, STBI_rgb_alpha);

		if (!pixels)
		{
			KITA_CORE_ERROR("VulkanTextureLoader: failed to load texture '{0}', reason: {1}",normalizedPath.string(),stbi_failure_reason() ? stbi_failure_reason() : "unknown");
			return false;
		}
		KITA_CORE_ASSERT(width > 0 && height > 0, "Loaded texture has invalid dimensions");

		const size_t pixelBytes = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;

		textureAsset.TexRawData.Width = width;
		textureAsset.TexRawData.Height = height;
		textureAsset.TexRawData.Channels = 4;
		textureAsset.TexRawData.Format = TexturePixelFormat::R8G8B8A8;

		textureAsset.TexRawData.Pixels.assign(pixels, pixels + pixelBytes);

		stbi_image_free(pixels);

		return true;
	}
	bool AssetBuilder::LoadMeshData_FBX(MeshAsset& meshAsset)
	{
		const std::filesystem::path meshPath = meshAsset.SourcePath.lexically_normal();
		meshAsset.MeshRawData.clear();

		if (!std::filesystem::exists(meshPath))
		{
			KITA_CORE_ERROR("Mesh asset file does not exist: {0}", meshPath.string());
			return false;
		}

		Assimp::Importer import;
		const aiScene* scene = import.ReadFile(meshPath.string(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

		if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode)
		{
			KITA_CORE_ERROR("assimp import error : {0}", import.GetErrorString());
			return false;
		}

		ProcessNode(scene->mRootNode, scene,meshAsset);
		return !meshAsset.MeshRawData.empty();
	}
	void AssetBuilder::ProcessNode(aiNode* node, const aiScene* scene, MeshAsset& meshAsset)
	{
		for (unsigned int i = 0; i < node->mNumMeshes; i++)
		{
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			ProcessMesh(mesh, scene, meshAsset);
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			ProcessNode(node->mChildren[i], scene, meshAsset);
		}
	}
	void AssetBuilder::ProcessMesh(aiMesh* mesh, const aiScene* scene, MeshAsset& meshAsset)
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
		MeshPrimitiveData data{};
		data.Vertices = vertices;
		data.Indices = indices;

		meshAsset.MeshRawData.push_back(data);
	}



}
