#include "renderer_pch.h"
#include "ThumbnailCache.h"

#include "asset/AssetManager.h"

#include <backends/imgui_impl_vulkan.h>

namespace Kita {

	ThumbnailCache::ThumbnailCache(VulkanResourceFactory& resourceFactory)
		: m_ResourceFactory(resourceFactory)
	{
	}

	ThumbnailCache::~ThumbnailCache()
	{
		Clear();
	}

	ThumbnailCache::ThumbnailHandle ThumbnailCache::GetOrCreate(AssetHandle handle, AssetType type, uint32_t preferredSize)
	{
		ProcessPendingReleases();

		if (!Asset::IsValidHandle(handle))
		{
			return {};
		}

		if (type == AssetType::MaterialInstance)
		{
			return GetOrCreateMaterialThumbnail(handle, preferredSize);
		}

		if (type != AssetType::Texture)
		{
			return {};
		}

		return GetOrCreateTextureThumbnail(handle, preferredSize);
	}

	void ThumbnailCache::ProcessRenderRequests(uint32_t maxRequests)
	{
		ProcessPendingReleases();

		if (!m_AssetPreviewRenderer || maxRequests == 0)
		{
			return;
		}

		uint32_t processedCount = 0;
		while (processedCount < maxRequests && !m_PendingPreviewOrder.empty())
		{
			const PreviewThumbnailKey key = m_PendingPreviewOrder.front();
			m_PendingPreviewOrder.pop_front();

			auto requestIt = m_PendingPreviewRequests.find(key);
			if (requestIt == m_PendingPreviewRequests.end())
			{
				continue;
			}

			AssetPreviewRequest request = requestIt->second;
			m_PendingPreviewRequests.erase(requestIt);

			if (m_AssetPreviewRenderer->TryGetCached(request).IsValid())
			{
				continue;
			}

			m_AssetPreviewRenderer->Render(request);
			++processedCount;
		}
	}

	void ThumbnailCache::Invalidate(AssetHandle handle)
	{
		ProcessPendingReleases();

		auto it = m_Cache.find(handle);
		if (it != m_Cache.end())
		{
			RetireThumbnail(it->second);
			m_Cache.erase(it);
		}

		for (auto requestIt = m_PendingPreviewRequests.begin(); requestIt != m_PendingPreviewRequests.end();)
		{
			if (requestIt->first.Handle != handle)
			{
				++requestIt;
				continue;
			}

			requestIt = m_PendingPreviewRequests.erase(requestIt);
		}

		m_PendingPreviewOrder.erase(
			std::remove_if(
				m_PendingPreviewOrder.begin(),
				m_PendingPreviewOrder.end(),
				[handle](const PreviewThumbnailKey& key)
				{
					return key.Handle == handle;
				}),
			m_PendingPreviewOrder.end());

		if (m_AssetPreviewRenderer)
		{
			m_AssetPreviewRenderer->Invalidate(handle);
		}
	}

	void ThumbnailCache::Clear()
	{
		if (m_Cache.empty() && m_RetiredThumbnails.empty() && m_PendingPreviewOrder.empty() && m_PendingPreviewRequests.empty())
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
		m_PendingPreviewOrder.clear();
		m_PendingPreviewRequests.clear();
	}

	ThumbnailCache::ThumbnailHandle ThumbnailCache::GetOrCreateTextureThumbnail(AssetHandle handle, uint32_t preferredSize)
	{
		if (IsHeavyTexturePreview(handle))
		{
			return GetOrCreateCubemapThumbnail(handle, preferredSize);
		}

		auto it = m_Cache.find(handle);
		if (it != m_Cache.end())
		{
			return {
				it->second.TextureID,
				it->second.Texture ? it->second.Texture->GetWidth() : 0,
				it->second.Texture ? it->second.Texture->GetHeight() : 0
			};
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

	ThumbnailCache::ThumbnailHandle ThumbnailCache::GetOrCreateCubemapThumbnail(AssetHandle handle, uint32_t size)
	{
		AssetPreviewRequest request{};
		request.Handle = handle;
		request.Type = AssetPreviewType::CubemapSphere;
		request.Size = size;
		return GetOrQueuePreview(request);
	}

	ThumbnailCache::ThumbnailHandle ThumbnailCache::GetOrCreateMaterialThumbnail(AssetHandle handle, uint32_t size)
	{
		AssetPreviewRequest request{};
		request.Handle = handle;
		request.Type = AssetPreviewType::MaterialSphere;
		request.Size = size;
		return GetOrQueuePreview(request);
	}

	ThumbnailCache::ThumbnailHandle ThumbnailCache::GetOrQueuePreview(const AssetPreviewRequest& request)
	{
		if (!m_AssetPreviewRenderer)
		{
			return {};
		}

		if (PreviewThumbnailHandle preview = m_AssetPreviewRenderer->TryGetCached(request); preview.IsValid())
		{
			return { preview.TextureID, preview.Width, preview.Height };
		}

		QueuePreviewRequest(request);
		return {};
	}

	bool ThumbnailCache::IsHeavyTexturePreview(AssetHandle handle) const
	{
		Ref<TextureAsset> textureAsset = AssetManager::GetInstance().GetTextureAsset(handle);
		if (!textureAsset)
		{
			return false;
		}

		return textureAsset->ImportSettings.Shape == TextureShape::TextureCube;
	}

	void ThumbnailCache::QueuePreviewRequest(const AssetPreviewRequest& request)
	{
		if (!Asset::IsValidHandle(request.Handle))
		{
			return;
		}

		PreviewThumbnailKey key{};
		key.Handle = request.Handle;
		key.Type = request.Type;
		key.Size = std::clamp(request.Size, 32u, 512u);
		key.Revision = request.Revision;

		if (m_PendingPreviewRequests.find(key) != m_PendingPreviewRequests.end())
		{
			return;
		}

		AssetPreviewRequest normalizedRequest = request;
		normalizedRequest.Size = key.Size;
		m_PendingPreviewOrder.push_back(key);
		m_PendingPreviewRequests.emplace(key, normalizedRequest);
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
