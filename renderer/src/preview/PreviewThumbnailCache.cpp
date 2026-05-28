#include "renderer_pch.h"
#include "PreviewThumbnailCache.h"

#include "core/Application.h"
#include "render/VulkanContext.h"
#include "render/VulkanImage.h"

#include <backends/imgui_impl_vulkan.h>

namespace Kita {

	namespace
	{
		template<typename T>
		void HashCombine(size_t& seed, const T& value)
		{
			std::hash<T> hasher;
			seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}
	}

	size_t PreviewThumbnailKeyHasher::operator()(const PreviewThumbnailKey& key) const
	{
		size_t seed = 0;
		HashCombine(seed, key.Handle);
		HashCombine(seed, static_cast<uint32_t>(key.Type));
		HashCombine(seed, key.Size);
		HashCombine(seed, key.Revision);
		return seed;
	}

	PreviewThumbnailCache::~PreviewThumbnailCache()
	{
		Clear();
	}

	PreviewThumbnailHandle PreviewThumbnailCache::StoreRenderedImage(
		const PreviewThumbnailKey& key,
		const VulkanImage& image,
		uint32_t width,
		uint32_t height)
	{
		ProcessPendingReleases();

		if (!Asset::IsValidHandle(key.Handle) || width == 0 || height == 0)
		{
			return {};
		}

		if (!image.IsValid() || !image.HasSampler() || image.GetView() == VK_NULL_HANDLE)
		{
			return {};
		}

		if (auto existing = m_Cache.find(key); existing != m_Cache.end())
		{
			return existing->second.Handle;
		}

		VulkanContext* context = image.GetContext();
		if (!context)
		{
			return {};
		}

		VulkanImage::CreateInfo imageInfo{};
		imageInfo.Name = "PreviewThumbnail_" + std::to_string(key.Handle) + "_" + std::to_string(key.Size);
		imageInfo.Type = VK_IMAGE_TYPE_2D;
		imageInfo.ViewType = VK_IMAGE_VIEW_TYPE_2D;
		imageInfo.Format = image.GetFormat();
		imageInfo.Extent = { width, height, 1 };
		imageInfo.MipLevels = 1;
		imageInfo.ArrayLayers = 1;
		imageInfo.Samples = VK_SAMPLE_COUNT_1_BIT;
		imageInfo.Tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.Usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		imageInfo.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		imageInfo.CreateSampler = true;
		imageInfo.MinFilter = VK_FILTER_LINEAR;
		imageInfo.MagFilter = VK_FILTER_LINEAR;
		imageInfo.AddressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		imageInfo.AddressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		imageInfo.AddressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

		auto thumbnailImage = CreateUnique<VulkanImage>(*context, imageInfo);
		if (!thumbnailImage || !thumbnailImage->IsValid())
		{
			return {};
		}

		VkCommandBuffer commandBuffer = context->GetCurrentCommandBuffer();
		if (commandBuffer == VK_NULL_HANDLE)
		{
			return {};
		}

		ImTextureID textureID = static_cast<ImTextureID>(reinterpret_cast<uint64_t>(
			ImGui_ImplVulkan_AddTexture(
				thumbnailImage->GetSampler(),
				thumbnailImage->GetView(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)));

		if (!textureID)
		{
			return {};
		}

		VulkanImage& mutableSource = const_cast<VulkanImage&>(image);
		const VkImageLayout sourcePreviousLayout = mutableSource.GetCurrentLayout();
		mutableSource.TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);
		thumbnailImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

		VkImageCopy copyRegion{};
		copyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.srcSubresource.mipLevel = 0;
		copyRegion.srcSubresource.baseArrayLayer = 0;
		copyRegion.srcSubresource.layerCount = 1;
		copyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		copyRegion.dstSubresource.mipLevel = 0;
		copyRegion.dstSubresource.baseArrayLayer = 0;
		copyRegion.dstSubresource.layerCount = 1;
		copyRegion.extent = { width, height, 1 };

		vkCmdCopyImage(
			commandBuffer,
			mutableSource.GetHandle(),
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			thumbnailImage->GetHandle(),
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&copyRegion);

		thumbnailImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		if (sourcePreviousLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL)
		{
			mutableSource.TransitionLayout(commandBuffer, sourcePreviousLayout);
		}

		CachedThumbnail thumbnail{};
		thumbnail.Key = key;
		thumbnail.Handle.TextureID = textureID;
		thumbnail.Handle.Width = width;
		thumbnail.Handle.Height = height;
		thumbnail.Image = std::move(thumbnailImage);

		PreviewThumbnailHandle handle = thumbnail.Handle;
		m_Cache.emplace(key, std::move(thumbnail));
		return handle;
	}

	PreviewThumbnailHandle PreviewThumbnailCache::Find(const PreviewThumbnailKey& key)
	{
		ProcessPendingReleases();

		auto it = m_Cache.find(key);
		if (it == m_Cache.end())
		{
			return {};
		}

		return it->second.Handle;
	}

	void PreviewThumbnailCache::Invalidate(AssetHandle handle)
	{
		ProcessPendingReleases();

		for (auto it = m_Cache.begin(); it != m_Cache.end();)
		{
			if (it->first.Handle != handle)
			{
				++it;
				continue;
			}

			RetireThumbnail(it->second);
			it = m_Cache.erase(it);
		}
	}

	void PreviewThumbnailCache::Invalidate(const PreviewThumbnailKey& key)
	{
		ProcessPendingReleases();

		auto it = m_Cache.find(key);
		if (it == m_Cache.end())
		{
			return;
		}

		RetireThumbnail(it->second);
		m_Cache.erase(it);
	}

	void PreviewThumbnailCache::Clear()
	{
		if (m_Cache.empty() && m_RetiredThumbnails.empty())
		{
			return;
		}

		Application::Get().GetVulkanContext().WaitIdle();

		for (auto& [key, thumbnail] : m_Cache)
		{
			(void)key;
			ReleaseThumbnail(thumbnail);
		}
		m_Cache.clear();

		for (auto& retired : m_RetiredThumbnails)
		{
			ReleaseThumbnail(retired.Thumbnail);
		}
		m_RetiredThumbnails.clear();
	}

	void PreviewThumbnailCache::ProcessPendingReleases()
	{
		if (m_RetiredThumbnails.empty())
		{
			return;
		}

		const uint64_t currentFrame = Application::Get().GetTimeSystem().GetFrameIndex();
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

	void PreviewThumbnailCache::RetireThumbnail(CachedThumbnail& thumbnail)
	{
		if (!thumbnail.Handle.TextureID)
		{
			ReleaseThumbnail(thumbnail);
			return;
		}

		const uint64_t currentFrame = Application::Get().GetTimeSystem().GetFrameIndex();
		const uint64_t safeDelayFrames =
			static_cast<uint64_t>(std::max(2u, Application::Get().GetVulkanContext().GetFramesInFlight())) + 1ull;

		RetiredThumbnail retired{};
		retired.Thumbnail = std::move(thumbnail);
		retired.ReleaseAfterFrame = currentFrame + safeDelayFrames;
		m_RetiredThumbnails.push_back(std::move(retired));

		thumbnail.Handle = {};
		thumbnail.Key = {};
		thumbnail.Image = nullptr;
	}

	void PreviewThumbnailCache::ReleaseThumbnail(CachedThumbnail& thumbnail)
	{
		if (thumbnail.Handle.TextureID)
		{
			ImGui_ImplVulkan_RemoveTexture(
				reinterpret_cast<VkDescriptorSet>(static_cast<uint64_t>(thumbnail.Handle.TextureID)));
		}

		thumbnail.Handle = {};
		thumbnail.Key = {};
		thumbnail.Image = nullptr;
	}

}
