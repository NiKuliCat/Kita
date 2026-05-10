#pragma once
#include <filesystem>
#include <glm/glm.hpp>
#include "core/UUID.h"
#include "render/mesh/Mesh.h"
#include "render/VulkanMaterial.h"
#include "render/VulkanTexture.h"

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


	struct Asset
	{
		Asset()
			:m_Handle(GenerateAssetHandle()) 
		{
		}

		virtual ~Asset() = default;
		virtual AssetType GetType() const = 0;

		bool IsValid() const { return m_Handle != InvalidAssetHandle; }
		static bool IsValidHandle(AssetHandle handle) { return handle != InvalidAssetHandle; }
		static AssetHandle GenerateAssetHandle() { return static_cast<AssetHandle>(UUID()); }

		AssetHandle m_Handle = InvalidAssetHandle;
	};

	struct MaterialAsset : public Asset
	{
		virtual AssetType GetType() const override { return AssetType::Material; }
		AssetHandle ShaderHandle = InvalidAssetHandle;
		AssetHandle AlbedoTextureHandle = InvalidAssetHandle;
		glm::vec4 BaseColor = glm::vec4(1.0f);
	};

	struct ShaderStageBinary
	{
		std::vector<uint8_t> Spirv;
		std::string EntryPoint = "main";
		bool Valid() const { return !Spirv.empty(); }
	};
	struct MeshPrimitiveData
	{
		std::vector<Vertex> Vertices;
		std::vector<uint32_t> Indices;
	};

	struct TexturePrimitiveData
	{
		uint32_t Width = 0;
		uint32_t Height = 0;
		uint32_t Channels = 0;
		std::vector<uint8_t> Pixels;
	};

	struct ShaderAsset : public Asset
	{
		virtual AssetType GetType() const override { return AssetType::Shader; }
		std::filesystem::path SourcePath;
		ShaderStageBinary VertexStage;
		ShaderStageBinary FragmentStage;
	};

	struct MeshAsset : Asset
	{
		virtual AssetType GetType() const override { return AssetType::Mesh; }
		std::filesystem::path SourcePath;
		std::vector<MeshPrimitiveData> MeshRawData;
	};

	struct TextureAsset : Asset
	{
		virtual AssetType GetType() const override { return AssetType::Texture; }
		std::filesystem::path SourcePath;
		TexturePrimitiveData  TexRawData;
	};


}
