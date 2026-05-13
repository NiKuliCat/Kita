#include "renderer_pch.h"
#include "EditorViewportSurface.h"

#include "render/VulkanBuffer.h"
#include "render/VulkanContext.h"
#include "render/VulkanImage.h"

#include <backends/imgui_impl_vulkan.h>

namespace Kita {

	namespace
	{
		void VKCheck(VkResult result, const char* message)
		{
			if (result != VK_SUCCESS)
			{
				KITA_CORE_ERROR("{0}, VkResult = {1}", message, static_cast<int32_t>(result));
				throw std::runtime_error(message);
			}
		}

		VkCommandBuffer BeginSingleTimeCommands(VulkanContext& context)
		{
			VkCommandBufferAllocateInfo allocInfo{};
			allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			allocInfo.commandPool = context.GetCommandPool();
			allocInfo.commandBufferCount = 1;

			VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
			VKCheck(
				vkAllocateCommandBuffers(context.GetDevice(), &allocInfo, &commandBuffer),
				"Failed to allocate EditorViewportSurface single-time command buffer");

			VkCommandBufferBeginInfo beginInfo{};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

			VKCheck(
				vkBeginCommandBuffer(commandBuffer, &beginInfo),
				"Failed to begin EditorViewportSurface single-time command buffer");

			return commandBuffer;
		}

		void EndSingleTimeCommands(VulkanContext& context, VkCommandBuffer commandBuffer)
		{
			VKCheck(
				vkEndCommandBuffer(commandBuffer),
				"Failed to end EditorViewportSurface single-time command buffer");

			VkSubmitInfo submitInfo{};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &commandBuffer;

			VKCheck(
				vkQueueSubmit(context.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE),
				"Failed to submit EditorViewportSurface single-time command buffer");

			VKCheck(
				vkQueueWaitIdle(context.GetGraphicsQueue()),
				"Failed to wait for EditorViewportSurface graphics queue idle");

			vkFreeCommandBuffers(context.GetDevice(), context.GetCommandPool(), 1, &commandBuffer);
		}

		VulkanRenderTarget::CreateInfo BuildRenderTargetCreateInfo( const EditorViewportSurface::CreateInfo& createInfo )
		{
			VulkanRenderTarget::CreateInfo rtInfo{};
			rtInfo.Name = createInfo.Name;
			rtInfo.Width = createInfo.Width;
			rtInfo.Height = createInfo.Height;
			rtInfo.Samples = createInfo.Samples;

			VulkanRenderTarget::ColorAttachmentDesc colorAttachment{};
			colorAttachment.Name = createInfo.Name + "_Color";
			colorAttachment.Format = createInfo.ColorFormat;
			colorAttachment.CreateSampler = true;
			colorAttachment.CreateResolveImage = false;
			colorAttachment.Filter = createInfo.SamplerFilter;
			colorAttachment.AddressMode = createInfo.SamplerAddressMode;
			rtInfo.ColorAttachments.push_back(colorAttachment);

			rtInfo.DepthAttachment.Enabled = true;
			rtInfo.DepthAttachment.Name = createInfo.Name + "_Depth";
			rtInfo.DepthAttachment.Format = createInfo.DepthFormat;
			rtInfo.DepthAttachment.CreateSampler = false;

			return rtInfo;
		}

		VulkanRenderTarget::CreateInfo BuildPickingRenderTargetCreateInfo(const EditorViewportSurface::CreateInfo& createInfo)
		{
			VulkanRenderTarget::CreateInfo rtInfo{};
			rtInfo.Name = createInfo.Name + "_Picking";
			rtInfo.Width = createInfo.Width;
			rtInfo.Height = createInfo.Height;
			rtInfo.Samples = VK_SAMPLE_COUNT_1_BIT;

			VulkanRenderTarget::ColorAttachmentDesc colorAttachment{};
			colorAttachment.Name = rtInfo.Name + "_Color";
			colorAttachment.Format = VK_FORMAT_R32_UINT;
			colorAttachment.ExtraUsage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			colorAttachment.CreateSampler = false;
			colorAttachment.CreateResolveImage = false;
			rtInfo.ColorAttachments.push_back(colorAttachment);

			rtInfo.DepthAttachment.Enabled = true;
			rtInfo.DepthAttachment.Name = rtInfo.Name + "_Depth";
			rtInfo.DepthAttachment.Format = createInfo.DepthFormat;
			rtInfo.DepthAttachment.CreateSampler = false;

			return rtInfo;
		}
	}

	EditorViewportSurface::EditorViewportSurface(VulkanContext& context,const CreateInfo& createInfo)
	{
		Init(context, createInfo);
	}

	EditorViewportSurface::~EditorViewportSurface()
	{
		Destroy();
	}

	void EditorViewportSurface::Init(VulkanContext& context, const CreateInfo& createInfo)
	{
		Destroy();

		m_Context = &context;
		m_CreateInfo = createInfo;
		m_CreateInfo.Width = std::max(1u, m_CreateInfo.Width);
		m_CreateInfo.Height = std::max(1u, m_CreateInfo.Height);

		CreateRenderTarget();
		CreatePickingResources();
		RecreateTextureID();
	}

	void EditorViewportSurface::Destroy()
	{
		if (m_Context)
		{
			m_Context->WaitIdle();
		}

		ReleaseTextureID();
		DestroyPickingResources();
		m_RenderTarget.reset();
		m_Context = nullptr;
		m_CreateInfo = {};
	}

	void EditorViewportSurface::EnsureSize(uint32_t width, uint32_t height)
	{
		width = std::max(1u, width);
		height = std::max(1u, height);

		if (!m_RenderTarget)
			return;

		if (width == m_RenderTarget->GetWidth() && height == m_RenderTarget->GetHeight())
			return;

		Resize(width, height);
	}


	void EditorViewportSurface::Resize(uint32_t width, uint32_t height)
	{
		KITA_CORE_ASSERT(m_Context, "EditorViewportSurface context is null");
		KITA_CORE_ASSERT(m_RenderTarget, "EditorViewportSurface render target is null");

		width = std::max(1u, width);
		height = std::max(1u, height);

		if (width == m_RenderTarget->GetWidth() && height == m_RenderTarget->GetHeight())
			return;

		m_Context->WaitIdle();

		ReleaseTextureID();

		m_CreateInfo.Width = width;
		m_CreateInfo.Height = height;
		m_RenderTarget->Resize(width, height);
		DestroyPickingResources();
		CreatePickingResources();

		RecreateTextureID();
	}

	VulkanRenderTarget& EditorViewportSurface::GetRenderTarget()
	{
		KITA_CORE_ASSERT(m_RenderTarget, "EditorViewportSurface render target is null");
		return *m_RenderTarget;
	}

	const VulkanRenderTarget& EditorViewportSurface::GetRenderTarget() const
	{
		KITA_CORE_ASSERT(m_RenderTarget, "EditorViewportSurface render target is null");
		return *m_RenderTarget;
	}

	VulkanRenderTarget& EditorViewportSurface::GetPickingRenderTarget()
	{
		KITA_CORE_ASSERT(m_PickingRenderTarget, "EditorViewportSurface picking render target is null");
		return *m_PickingRenderTarget;
	}

	const VulkanRenderTarget& EditorViewportSurface::GetPickingRenderTarget() const
	{
		KITA_CORE_ASSERT(m_PickingRenderTarget, "EditorViewportSurface picking render target is null");
		return *m_PickingRenderTarget;
	}

	uint32_t EditorViewportSurface::ReadPickingPixel(uint32_t x, uint32_t y)
	{
		KITA_CORE_ASSERT(m_Context, "EditorViewportSurface context is null");
		KITA_CORE_ASSERT(m_PickingRenderTarget, "EditorViewportSurface picking render target is null");
		KITA_CORE_ASSERT(m_PickingReadbackBuffer, "EditorViewportSurface picking readback buffer is null");

		if (x >= m_CreateInfo.Width || y >= m_CreateInfo.Height)
			return 0;

		m_Context->WaitIdle();

		VulkanImage& pickingImage = const_cast<VulkanImage&>(m_PickingRenderTarget->GetColorAttachment(0));
		const VkImageLayout previousLayout = pickingImage.GetCurrentLayout();

		VkCommandBuffer commandBuffer = BeginSingleTimeCommands(*m_Context);

		pickingImage.TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL);

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;
		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;
		region.imageOffset = {
			static_cast<int32_t>(x),
			static_cast<int32_t>(y),
			0
		};
		region.imageExtent = { 1, 1, 1 };

		vkCmdCopyImageToBuffer(
			commandBuffer,
			pickingImage.GetHandle(),
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			m_PickingReadbackBuffer->GetHandle(),
			1,
			&region);

		if (previousLayout != VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL &&
			previousLayout != VK_IMAGE_LAYOUT_UNDEFINED)
		{
			pickingImage.TransitionLayout(commandBuffer, previousLayout);
		}

		EndSingleTimeCommands(*m_Context, commandBuffer);

		m_PickingReadbackBuffer->Invalidate(sizeof(uint32_t), 0);

		const bool wasMapped = m_PickingReadbackBuffer->IsMapped();
		void* mappedData = m_PickingReadbackBuffer->Map(sizeof(uint32_t), 0);
		KITA_CORE_ASSERT(mappedData, "EditorViewportSurface failed to map picking readback buffer");

		uint32_t pickedValue = *static_cast<const uint32_t*>(mappedData);

		if (!wasMapped)
			m_PickingReadbackBuffer->Unmap();

		return pickedValue;
	}

	void EditorViewportSurface::CreateRenderTarget()
	{
		KITA_CORE_ASSERT(m_Context, "EditorViewportSurface context is null");

		VulkanRenderTarget::CreateInfo rtInfo = BuildRenderTargetCreateInfo(m_CreateInfo);
		m_RenderTarget = CreateUnique<VulkanRenderTarget>(*m_Context, rtInfo);
	}

	void EditorViewportSurface::CreatePickingResources()
	{
		KITA_CORE_ASSERT(m_Context, "EditorViewportSurface context is null");
		VulkanRenderTarget::CreateInfo pickingRtInfo = BuildPickingRenderTargetCreateInfo(m_CreateInfo);
		m_PickingRenderTarget = CreateUnique<VulkanRenderTarget>(*m_Context, pickingRtInfo);

		VulkanBuffer::CreateInfo readbackBufferInfo{};
		readbackBufferInfo.Name = m_CreateInfo.Name + "_PickingReadback";
		readbackBufferInfo.Size = sizeof(uint32_t);
		readbackBufferInfo.Usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
		readbackBufferInfo.MemoryProperties =
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

		m_PickingReadbackBuffer = CreateUnique<VulkanBuffer>(*m_Context, readbackBufferInfo);
	}

	void EditorViewportSurface::DestroyPickingResources()
	{
		m_PickingReadbackBuffer.reset();
		m_PickingRenderTarget.reset();
	}

	void EditorViewportSurface::RecreateTextureID()
	{
		ReleaseTextureID();

		if (!m_RenderTarget)
			return;

		const VulkanImage& sampledImage = m_RenderTarget->GetSampledColorAttachment(0);
		KITA_CORE_ASSERT(sampledImage.HasSampler(), "EditorViewportSurface sampled image has no sampler");
		KITA_CORE_ASSERT(sampledImage.GetView() != VK_NULL_HANDLE, "EditorViewportSurface sampled image view is null");

		m_TextureID = static_cast<ImTextureID>(reinterpret_cast<uint64_t>(
			ImGui_ImplVulkan_AddTexture(
				sampledImage.GetSampler(),
				sampledImage.GetView(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)));
	}

	void EditorViewportSurface::ReleaseTextureID()
	{
		if (!m_TextureID)
			return;

		ImGui_ImplVulkan_RemoveTexture(
			reinterpret_cast<VkDescriptorSet>(static_cast<uint64_t>(m_TextureID)));
		m_TextureID = 0;
	}

}
