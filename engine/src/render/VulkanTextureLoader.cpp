#include "kita_pch.h"
#include "VulkanTextureLoader.h"
#include "render/VulkanTexture.h"
#include "render/VulkanContext.h"
#include "core/Log.h"

namespace Kita {

	Ref<VulkanTexture> VulkanTextureLoader::LoadTexture2D(
		VulkanContext& context,
		const TextureAsset& texAsset)
	{
		
		TexturePrimitiveData data = texAsset.TexRawData;

		const VkDeviceSize imageSize =
			static_cast<VkDeviceSize>(data.Width) *
			static_cast<VkDeviceSize>(data.Height) * 4;

		VulkanTexture::CreateInfo textureInfo{};
		textureInfo.Name = texAsset.SourcePath.filename().string();
		textureInfo.Width = static_cast<uint32_t>(data.Width);
		textureInfo.Height = static_cast<uint32_t>(data.Height);
		textureInfo.Format = VK_FORMAT_R8G8B8A8_UNORM;
		textureInfo.CreateSampler = true;
		textureInfo.Filter = VK_FILTER_LINEAR;
		textureInfo.AddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		textureInfo.MaxAnisotropy = 1.0f;
		textureInfo.EnableMipmaps = false;
		textureInfo.MipLevels = 1;
		textureInfo.PixelData = data.Pixels.data();
		textureInfo.PixelDataSize = imageSize;

		Ref<VulkanTexture> texture = nullptr;

		try
		{
			texture = CreateRef<VulkanTexture>(context, textureInfo);
		}
		catch (const std::exception& e)
		{
			KITA_CORE_ERROR("VulkanTextureLoader: failed to create VulkanTexture from '{0}': {1}",texAsset.SourcePath.string(),e.what());
		}

		return texture;
	}

}
