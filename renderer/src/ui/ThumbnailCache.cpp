#include "renderer_pch.h"
#include "ThumbnailCache.h"

#include <backends/imgui_impl_vulkan.h>
namespace Kita {



	ThumbnailCache::ThumbnailCache(VulkanResourceFactory& resourceFactory)
		:m_ResourceFactory(resourceFactory)
	{

	}

	ThumbnailCache::~ThumbnailCache()
	{
		Clear();
	}

	ThumbnailCache::ThumbnailHandle ThumbnailCache::GetOrCreate(AssetHandle handle, AssetType type)
	{
		if (!Asset::IsValidHandle(handle))
		{
			return {};
		}

		if (type != AssetType::Texture)
		{
			return {};
		}

		return GetOrCreateTextureThumbnail(handle);
	}

	void ThumbnailCache::Invalidate(AssetHandle handle)
	{
		auto it = m_Cache.find(handle);
		if (it == m_Cache.end())
		{
			return;
		}

		ReleaseThumbnail(it->second);
		m_Cache.erase(it);
	}

	void ThumbnailCache::Clear()
	{
		if (m_Cache.empty())
		{
			return;
		}

		Application::Get().GetVulkanContext().WaitIdle();

		for (auto& [handle, thumbnail] : m_Cache)
		{
			(void)handle;
			ReleaseThumbnail(thumbnail);
		}

		m_Cache.clear();
	}

	ThumbnailCache::ThumbnailHandle ThumbnailCache::GetOrCreateTextureThumbnail(AssetHandle handle)
	{
		auto it = m_Cache.find(handle);
		if (it != m_Cache.end())
		{
			return { it->second.TextureID, it->second.Texture ? it->second.Texture->GetWidth() : 0, it->second.Texture ? it->second.Texture->GetHeight() : 0 };
		}

		Ref<VulkanTexture> texture = m_ResourceFactory.GetOrCreateTexture(handle);
		if (!texture || !texture->IsValid())
		{
			return {};
		}

		const VulkanImage& image = texture->GetImage();
		if (!image.HasSampler() || image.GetView() == VK_NULL_HANDLE)
		{
			return {};
		}

		ImTextureID textureID = static_cast<ImTextureID>(reinterpret_cast<uint64_t>(
			ImGui_ImplVulkan_AddTexture(
				image.GetSampler(),
				image.GetView(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)));

		if (!textureID)
		{
			return {};
		}

		CachedThumbnail thumbnail{};
		thumbnail.Handle = handle;
		thumbnail.Type = AssetType::Texture;
		thumbnail.Texture = texture;
		thumbnail.TextureID = textureID;

		m_Cache.emplace(handle, thumbnail);
		return { textureID, texture->GetWidth(), texture->GetHeight() };
	}

	void ThumbnailCache::ReleaseThumbnail(CachedThumbnail& thumbnail)
	{
		if (thumbnail.TextureID)
		{
			ImGui_ImplVulkan_RemoveTexture(
				reinterpret_cast<VkDescriptorSet>(static_cast<uint64_t>(thumbnail.TextureID)));
			thumbnail.TextureID = 0;
		}

		thumbnail.Texture = nullptr;
		thumbnail.Handle = InvalidAssetHandle;
		thumbnail.Type = AssetType::None;
	}

}
