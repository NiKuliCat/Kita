#pragma once
#include "core/Core.h"
#include "render/mesh/Mesh.h"
#include "render/VulkanMaterial.h"
#include "render/BufferLayout.h"
#include "asset/Asset.h"
namespace Kita {


	struct MeshRenderer
	{
		AssetHandle MeshAssetHandle = InvalidAssetHandle;
		AssetHandle DefaultMaterialAssetHandle = InvalidAssetHandle;
		std::vector<AssetHandle> MaterialAssetHandles;
	};

}
