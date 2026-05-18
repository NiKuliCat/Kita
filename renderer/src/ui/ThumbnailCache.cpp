#include "renderer_pch.h"
#include "ThumbnailCache.h"

#include "asset/AssetManager.h"

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
		ProcessPendingReleases();

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
		ProcessPendingReleases();

		auto it = m_Cache.find(handle);
		if (it == m_Cache.end())
		{
			return;
		}

		RetireThumbnail(it->second);
		m_Cache.erase(it);
	}

	void ThumbnailCache::Clear()
	{
		if (m_Cache.empty() && m_RetiredThumbnails.empty())
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

		for (auto& retired : m_RetiredThumbnails)
		{
			ReleaseThumbnail(retired.Thumbnail);
		}

		m_RetiredThumbnails.clear();
	}

	ThumbnailCache::ThumbnailHandle ThumbnailCache::GetOrCreateTextureThumbnail(AssetHandle handle)
	{
		auto it = m_Cache.find(handle);
		if (it != m_Cache.end())
		{
			return { it->second.TextureID, it->second.Texture ? it->second.Texture->GetWidth() : 0, it->second.Texture ? it->second.Texture->GetHeight() : 0 };
		}

		TextureImportSettings importSettings{};
		if (AssetManager::GetInstance().GetTextureImportSettings(handle, importSettings) &&
			importSettings.Shape == TextureShape::TextureCube)
		{
			return {};
		}

		Ref<VulkanTexture> texture = m_ResourceFactory.GetOrCreateTexture(handle);
		if (!texture || !texture->IsValid())
		{
			return {};
		}

		if (texture->GetType() != TextureType::Texture2D)
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

	void ThumbnailCache::RetireThumbnail(CachedThumbnail& thumbnail)
	{
		if (!thumbnail.TextureID)
		{
			ReleaseThumbnail(thumbnail);
			return;
		}

		const uint64_t currentFrame =
			Application::Get().GetTimeSystem().GetFrameIndex();
		const uint64_t safeDelayFrames =
			static_cast<uint64_t>(std::max(2u, Application::Get().GetVulkanContext().GetFramesInFlight())) + 1ull;

		RetiredThumbnail retired{};
		retired.Thumbnail = thumbnail;
		retired.ReleaseAfterFrame = currentFrame + safeDelayFrames;
		m_RetiredThumbnails.push_back(std::move(retired));

		thumbnail.Texture = nullptr;
		thumbnail.TextureID = 0;
		thumbnail.Handle = InvalidAssetHandle;
		thumbnail.Type = AssetType::None;
	}

	void ThumbnailCache::ProcessPendingReleases()
	{
		if (m_RetiredThumbnails.empty())
		{
			return;
		}

		const uint64_t currentFrame =
			Application::Get().GetTimeSystem().GetFrameIndex();

		for (size_t index = 0; index < m_RetiredThumbnails.size();)
		{
			if (currentFrame < m_RetiredThumbnails[index].ReleaseAfterFrame)
			{
				++index;
				continue;
			}

			ReleaseThumbnail(m_RetiredThumbnails[index].Thumbnail);
			m_RetiredThumbnails.erase(m_RetiredThumbnails.begin() + static_cast<std::ptrdiff_t>(index));
		}
	}

}
