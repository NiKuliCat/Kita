#pragma once
#include "asset/Asset.h"
#include "render/ShaderCompiler.h"
struct aiMesh;
struct aiNode;
struct aiScene;
namespace Kita {

	class AssetFactory
	{
	public:
		static Ref<Asset> CreateFromMetadata(const AssetMetadata& metadata, const std::filesystem::path& assetPath);
	};


	class AssetBuilder
	{
	public:
		static bool CompilerShader(ShaderAsset& shaderAsset);
		static bool LoadPixel(TextureAsset& textureAsset);
		static bool LoadMeshData_FBX(MeshAsset& meshAsset);

	private:
		//shader loader helper
		static ShaderCompiler::CompileResult CompileShaderStageWithCache(ShaderCompiler& compiler, const ShaderCompiler::CompileRequest& request);

		// mesh loader helper
		static void ProcessNode(aiNode* node, const aiScene* scene, MeshAsset& meshAsset);
		static void ProcessMesh(aiMesh* mesh, const aiScene* scene, MeshAsset& meshAsset);

	};

}
