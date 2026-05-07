#include "kita_pch.h"
#include "Texture.h"
#include "core/Log.h"
namespace Kita {
	Ref<Texture> Texture::Create(const TextureDescriptor& descriptor, const std::string& path)
	{
		(void)descriptor;
		(void)path;
		throw std::runtime_error("Legacy Texture::Create path is disabled during Vulkan-only migration.");
		return nullptr;
	}
	Ref<Texture> Texture::CreateCubeMap(const TextureDescriptor& descriptor, const CubemapFacePaths& faces)
	{
		(void)descriptor;
		(void)faces;
		throw std::runtime_error("Legacy Texture::CreateCubeMap path is disabled during Vulkan-only migration.");
		return nullptr;
	}
}
