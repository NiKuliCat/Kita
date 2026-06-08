#pragma once
#include <filesystem>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include <glm/glm.hpp>

#include "core/UUID.h"
#include "render/mesh/Mesh.h"
#include "render/VulkanTexture.h"

namespace Kita {
	using AssetHandle = uint64_t;
	static constexpr AssetHandle InvalidAssetHandle = 0;
	enum class AssetType
	{
		None = 0,
		MaterialInstance = 1,
		Shader,
		Texture,
		Mesh,
		MaterialDefinition = 5,
		Material = MaterialInstance,
		ShaderLab = MaterialDefinition
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

	struct MaterialSurfaceParams
	{
		glm::vec4 BaseColor = glm::vec4(1.0f);
		glm::vec3 Emissive = glm::vec3(1.0f);

		float Metallic = 0.0f;
		float Roughness = 1.0f;
		float AmbientOcclusion = 1.0f;
		float Opacity = 1.0f;
		float NormalScale = 1.0f;
		float AlphaCutoff = 0.5f;
	};

	struct MaterialTextures
	{
		AssetHandle Albedo = InvalidAssetHandle;
		AssetHandle Normal = InvalidAssetHandle;
		AssetHandle MetallicRoughness = InvalidAssetHandle;
		AssetHandle AmbientOcclusion = InvalidAssetHandle;
		AssetHandle Emissive = InvalidAssetHandle;
		AssetHandle Opacity = InvalidAssetHandle;
	};

	enum class MaterialValueType : uint8_t
	{
		Bool,
		Int,
		Float,
		Float2,
		Float3,
		Float4,
		Color,
		Texture2D,
		TextureCube
	};

	struct MaterialPropertyValue
	{
		MaterialValueType ValueType = MaterialValueType::Float;
		std::variant<bool, int32_t, float, glm::vec2, glm::vec3, glm::vec4, AssetHandle, std::string> Data = 0.0f;
	};

	struct MaterialPropertyUIHint
	{
		bool HasRange = false;
		float MinValue = 0.0f;
		float MaxValue = 1.0f;
		float Step = 0.01f;
		bool HDR = false;
	};

	struct MaterialPropertyDesc
	{
		std::string Name;
		std::string DisplayName;
		MaterialValueType ValueType = MaterialValueType::Float;
		MaterialPropertyValue DefaultValue{};
		MaterialPropertyUIHint UIHint{};
	};

	struct MaterialPropertyBlock
	{
		std::unordered_map<std::string, MaterialPropertyValue> Values;
		std::unordered_map<std::string, MaterialPropertyValue> OrphanValues;
	};


	struct MaterialInstanceAsset : public Asset
	{
		virtual AssetType GetType() const override { return AssetType::MaterialInstance; }
		AssetHandle MaterialDefinitionHandle = InvalidAssetHandle;
		MaterialPropertyBlock PropertyBlock;
		AssetHandle ShaderHandle = InvalidAssetHandle;
		MaterialSurfaceParams m_SurfaceParams;
		MaterialTextures m_Textures;
	};

	using MaterialAsset = MaterialInstanceAsset;

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


	enum class TextureShape : uint8_t
	{
		Texture2D = 0,
		TextureCube
	};

	enum class TextureColorSpace : uint8_t
	{
		SRGB = 0,
		Linear
	};

	enum class TexturePixelFormat : uint8_t
	{
		Unknown = 0,
		R8G8B8A8,
		R16G16B16A16_Float,
		R32G32B32A32_Float
	};

	struct TextureImportSettings
	{
		TextureShape Shape = TextureShape::Texture2D;
		TextureColorSpace ColorSpace = TextureColorSpace::SRGB;
		bool GenerateMipmaps = true;
	};

	struct TexturePrimitiveData
	{
		uint32_t Width = 0;
		uint32_t Height = 0;
		uint32_t Channels = 0;
		TexturePixelFormat Format = TexturePixelFormat::Unknown;
		std::vector<uint8_t> Pixels;

		bool IsValid() const
		{
			return Width > 0 && Height > 0 && !Pixels.empty() && Format != TexturePixelFormat::Unknown;
		}

		bool IsHDR() const
		{
			return Format == TexturePixelFormat::R16G16B16A16_Float
				|| Format == TexturePixelFormat::R32G32B32A32_Float;
		}

		void Reset()
		{
			Width = 0;
			Height = 0;
			Channels = 0;
			Format = TexturePixelFormat::Unknown;
			Pixels.clear();
		}

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
		TextureImportSettings ImportSettings;
		TexturePrimitiveData  TexRawData;

		bool IsCube() const
		{
			return ImportSettings.Shape == TextureShape::TextureCube;
		}

		bool IsTexture2D() const
		{
			return ImportSettings.Shape == TextureShape::Texture2D;
		}

		bool IsHDRSource() const
		{
			return TexRawData.IsHDR();
		}

		bool IsValidSource() const
		{
			return !SourcePath.empty() && TexRawData.IsValid();
		}

		void ResetSourceData()
		{
			TexRawData.Reset();
		}
	};


}
