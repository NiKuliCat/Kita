#pragma once
#include <filesystem>
#include <glm/glm.hpp>
#include "core/UUID.h"
#include "render/mesh/Mesh.h"
#include "render/VulkanMaterial.h"
#include "render/VulkanTexture.h"

struct aiMesh;
struct aiNode;
struct aiScene;

namespace Kita {
	using AssetHandle = uint64_t;
	static constexpr AssetHandle InvalidAssetHandle = 0;
	enum class AssetType
	{
		None = 0,
		Material,
		Shader,
		Texture,
		Mesh
	};


	struct AssetMetadata
	{
		AssetHandle handle = 0;
		AssetType   type = AssetType::None;
		std::filesystem::path relativePath;
	};


	class Asset
	{
	public:
		Asset()
			:m_Handle(GenerateAssetHandle()) 
		{
		}

		virtual ~Asset() = default;

		virtual AssetType GetType() const = 0;

		AssetHandle GetHandle() const { return m_Handle; }
		void SetHandle(const AssetHandle& handle) { m_Handle = handle; }

		bool IsValid() const { return m_Handle != InvalidAssetHandle; }
		static bool IsValidHandle(AssetHandle handle) { return handle != InvalidAssetHandle; }

		static AssetHandle GenerateAssetHandle() { return static_cast<AssetHandle>(UUID()); }

	protected:
		AssetHandle m_Handle = InvalidAssetHandle;
	};


	class MaterialAsset : public Asset
	{
	public:
		MaterialAsset() = default;
		virtual ~MaterialAsset() = default;
		virtual AssetType GetType() const override { return AssetType::Material; }

		Ref<VulkanMaterial> CreateRuntimeMaterial() const;
		void ApplyToRuntimeMaterial(VulkanMaterial& material) const;

		AssetHandle ShaderHandle = InvalidAssetHandle;
		AssetHandle AlbedoTextureHandle = InvalidAssetHandle;
		glm::vec4 BaseColor = glm::vec4(1.0f);
	};

	class ShaderAsset : public Asset
	{
	public:
		struct StageBinary
		{
			std::vector<uint8_t> Spirv;
			std::string EntryPoint = "main";
			bool Valid() const { return !Spirv.empty(); }
		};

	public:
		ShaderAsset() = default;
		ShaderAsset(const std::filesystem::path& path)
		{
			SetShaderPath(path);
		}

		virtual AssetType GetType() const override { return AssetType::Shader; }

		void SetShaderPath(const std::filesystem::path& path);
		bool Compile();

		const std::filesystem::path& GetFilePath() const { return m_ShaderPath; }

		const StageBinary& GetVertexStage() const { return m_VertexStage; }
		const StageBinary& GetFragmentStage() const { return m_FragmentStage; }

	private:
		std::filesystem::path m_ShaderPath;
		StageBinary m_VertexStage;
		StageBinary m_FragmentStage;
	};



	class TextureAsset : public Asset
	{
	public:
		TextureAsset() = default;
		TextureAsset(const std::filesystem::path& path)
		{
			SetTexturePath(path);
		}
		virtual ~TextureAsset() = default;
		virtual AssetType GetType() const override { return AssetType::Texture; }

		const std::filesystem::path& GetFilePath() const { return m_TexturePath; }
		void SetTexturePath(const std::filesystem::path& path);

		const Ref<VulkanTexture>& GetRuntimeTexture() const { return m_RuntimeTexture; }

	private:
		std::filesystem::path m_TexturePath;
		Ref<VulkanTexture> m_RuntimeTexture = nullptr;
	};

	class MeshAsset : public Asset
	{
	public:
		MeshAsset() = default;
		virtual ~MeshAsset() = default;
		virtual AssetType GetType() const override { return AssetType::Mesh; }

		bool LoadFromFile(const std::filesystem::path& path);

		const std::filesystem::path& GetFilePath() const { return m_MeshPath; }
		const std::vector<Ref<Mesh>>& GetSubMeshes() const { return m_SubMeshes; }

	private:
		void ProcessNode(aiNode* node, const aiScene* scene);
		void ProcessMesh(aiMesh* mesh, const aiScene* scene);

	private:
		std::filesystem::path m_MeshPath;
		std::vector<Ref<Mesh>> m_SubMeshes;
	};

}
