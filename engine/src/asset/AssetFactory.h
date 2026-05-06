#pragma once
#include "asset/Asset.h"
#include "core/Log.h"
#include "serialize/MaterialSerializer.h"

namespace Kita {

	class AssetFactory
	{
	public:
		static Ref<Asset> CreateFromMetadata(
			const AssetMetadata& metadata,
			const std::filesystem::path& assetPath)
		{
			switch (metadata.type)
			{
			case AssetType::Shader:
			{
				Ref<ShaderAsset> shaderAsset = CreateRef<ShaderAsset>();
				shaderAsset->SetHandle(metadata.handle);
				shaderAsset->SetShaderPath(assetPath);
				return shaderAsset;
			}
			case AssetType::Texture:
			{
				Ref<TextureAsset> textureAsset = CreateRef<TextureAsset>();
				textureAsset->SetHandle(metadata.handle);
				textureAsset->SetTexturePath(assetPath);
				return textureAsset;
			}
			case AssetType::Material:
			{
				Ref<MaterialAsset> materialAsset = CreateRef<MaterialAsset>();
				materialAsset->SetHandle(metadata.handle);

				if (!MaterialSerializer::Deserialize(assetPath, *materialAsset))
				{
					KITA_CORE_WARN("Failed to deserialize material asset: {}", assetPath.string());
					return nullptr;
				}

				return materialAsset;
			}
			case AssetType::Mesh:
			{
				Ref<MeshAsset> meshAsset = CreateRef<MeshAsset>();
				meshAsset->SetHandle(metadata.handle);
				if (!meshAsset->LoadFromFile(assetPath))
				{
					KITA_CORE_WARN("Failed to load mesh asset: {}", assetPath.string());
					return nullptr;
				}

				return meshAsset;
			}
			default:
				KITA_CORE_WARN("AssetFactory unsupported asset type.");
				return nullptr;
			}
		}
	};

}
