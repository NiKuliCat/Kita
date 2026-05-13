#pragma once

#include "core/Core.h"
#include "asset/Asset.h"
#include <filesystem>

namespace Kita {

	class VulkanContext;
	class VulkanTexture;

	class VulkanTextureLoader
	{
	public:
		static Ref<VulkanTexture> LoadTexture2D(
			VulkanContext& context,
			const TextureAsset& texAsset);
	};

}

