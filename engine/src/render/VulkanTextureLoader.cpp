#include "kita_pch.h"
#include "VulkanTextureLoader.h"

#include "render/VulkanTexture.h"
#include "render/VulkanContext.h"
#include "core/Log.h"
#include "third_party/stb_image/stb_image.h"

namespace Kita {

	Ref<VulkanTexture> VulkanTextureLoader::LoadTexture2D(
		VulkanContext& context,
		const std::filesystem::path& path)
	{
		const std::filesystem::path normalizedPath = path.lexically_normal();

		if (!std::filesystem::exists(normalizedPath))
		{
			KITA_CORE_ERROR("VulkanTextureLoader: texture file does not exist: {0}", normalizedPath.string());
			return nullptr;
		}

		int width = 0;
		int height = 0;
		int channels = 0;

		stbi_set_flip_vertically_on_load(0);

		stbi_uc* pixels = stbi_load(
			normalizedPath.string().c_str(),
			&width,
			&height,
			&channels,
			STBI_rgb_alpha);

		if (!pixels)
		{
			KITA_CORE_ERROR(
				"VulkanTextureLoader: failed to load texture '{0}', reason: {1}",
				normalizedPath.string(),
				stbi_failure_reason() ? stbi_failure_reason() : "unknown");
			return nullptr;
		}

		KITA_CORE_ASSERT(width > 0 && height > 0, "Loaded texture has invalid dimensions");

		const VkDeviceSize imageSize =
			static_cast<VkDeviceSize>(width) *
			static_cast<VkDeviceSize>(height) * 4;

		VulkanTexture::CreateInfo textureInfo{};
		textureInfo.Name = normalizedPath.filename().string();
		textureInfo.Width = static_cast<uint32_t>(width);
		textureInfo.Height = static_cast<uint32_t>(height);
		textureInfo.Format = VK_FORMAT_R8G8B8A8_UNORM;
		textureInfo.CreateSampler = true;
		textureInfo.Filter = VK_FILTER_LINEAR;
		textureInfo.AddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		textureInfo.MaxAnisotropy = 1.0f;
		textureInfo.EnableMipmaps = false;
		textureInfo.MipLevels = 1;
		textureInfo.PixelData = pixels;
		textureInfo.PixelDataSize = imageSize;

		Ref<VulkanTexture> texture = nullptr;

		try
		{
			texture = CreateRef<VulkanTexture>(context, textureInfo);
		}
		catch (const std::exception& e)
		{
			KITA_CORE_ERROR(
				"VulkanTextureLoader: failed to create VulkanTexture from '{0}': {1}",
				normalizedPath.string(),
				e.what());
		}

		stbi_image_free(pixels);

		if (texture)
		{
			KITA_CORE_INFO(
				"Loaded Vulkan texture: {0} ({1}x{2}, channels={3})",
				normalizedPath.string(),
				width,
				height,
				channels);
		}

		return texture;
	}

}
