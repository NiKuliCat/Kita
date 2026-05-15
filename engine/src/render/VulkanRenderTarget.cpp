#include "kita_pch.h"
#include "VulkanRenderTarget.h"

#include "render/VulkanContext.h"
#include "core/Log.h"

namespace Kita {

	namespace
	{
		// Keep this translation unit explicitly participating in incremental builds.
		void VKCheck(VkResult result, const char* message)
		{
			if (result != VK_SUCCESS)
			{
				KITA_CORE_ERROR("{0}, VkResult = {1}", message, static_cast<int32_t>(result));
				throw std::runtime_error(message);
			}
		}

		std::string MakeAttachmentName(const std::string& baseName, const char* suffix, uint32_t index)
		{
			return baseName + "_" + suffix + std::to_string(index);
		}
	}

	VulkanRenderTarget::VulkanRenderTarget(VulkanContext& context, const CreateInfo& createInfo)
	{
		Init(context, createInfo);
	}

	VulkanRenderTarget::~VulkanRenderTarget()
	{
		Destroy();
	}

	VulkanRenderTarget::VulkanRenderTarget(VulkanRenderTarget&& other) noexcept
		: m_Context(other.m_Context),
		m_CreateInfo(std::move(other.m_CreateInfo)),
		m_ColorAttachments(std::move(other.m_ColorAttachments)),
		m_DepthAttachment(std::move(other.m_DepthAttachment)),
		m_IsRendering(other.m_IsRendering)
	{
		other.m_Context = nullptr;
		other.m_IsRendering = false;
	}

	VulkanRenderTarget& VulkanRenderTarget::operator=(VulkanRenderTarget&& other) noexcept
	{
		if (this == &other)
			return *this;

		Destroy();

		m_Context = other.m_Context;
		m_CreateInfo = std::move(other.m_CreateInfo);
		m_ColorAttachments = std::move(other.m_ColorAttachments);
		m_DepthAttachment = std::move(other.m_DepthAttachment);
		m_IsRendering = other.m_IsRendering;

		other.m_Context = nullptr;
		other.m_IsRendering = false;
		return *this;
	}

	void VulkanRenderTarget::Init(VulkanContext& context, const CreateInfo& createInfo)
	{
		Destroy();

		KITA_CORE_ASSERT(createInfo.Width > 0, "VulkanRenderTarget width must be > 0");
		KITA_CORE_ASSERT(createInfo.Height > 0, "VulkanRenderTarget height must be > 0");
		KITA_CORE_ASSERT(!createInfo.ColorAttachments.empty(), "VulkanRenderTarget requires at least one color attachment");

		m_Context = &context;
		m_CreateInfo = createInfo;
		m_IsRendering = false;

		CreateAttachments();

	}

	void VulkanRenderTarget::Destroy()
	{
		m_IsRendering = false;
		m_DepthAttachment.reset();
		m_ColorAttachments.clear();
		m_CreateInfo = {};
		m_Context = nullptr;
	}

	void VulkanRenderTarget::Resize(uint32_t width, uint32_t height)
	{
		KITA_CORE_ASSERT(m_Context, "VulkanRenderTarget context is null");

		if (width == 0 || height == 0)
			return;

		if (m_CreateInfo.Width == width && m_CreateInfo.Height == height)
			return;

		CreateInfo resizedInfo = m_CreateInfo;
		resizedInfo.Width = width;
		resizedInfo.Height = height;
		Init(*m_Context, resizedInfo);
	}

	void VulkanRenderTarget::BeginRendering(
		VkCommandBuffer commandBuffer,
		const std::vector<VkClearValue>& colorClearValues,
		const VkClearValue* depthClearValue,
		bool useDepthAttachment,
		bool clearColors,
		bool clearDepthAttachment)
	{
		KITA_CORE_ASSERT(m_Context, "VulkanRenderTarget context is null");
		KITA_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE, "VulkanRenderTarget BeginRendering commandBuffer is null");
		KITA_CORE_ASSERT(!m_IsRendering, "VulkanRenderTarget is already rendering");

		std::vector<VkRenderingAttachmentInfo> colorAttachments;
		colorAttachments.resize(m_ColorAttachments.size());

		for (size_t i = 0; i < m_ColorAttachments.size(); ++i)
		{
			ColorAttachment& colorAttachment = m_ColorAttachments[i];

			colorAttachment.Image.TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			VkRenderingAttachmentInfo attachmentInfo{};
			attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			attachmentInfo.imageView = colorAttachment.Image.GetView();
			attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachmentInfo.loadOp = clearColors ? colorAttachment.Desc.LoadOp : VK_ATTACHMENT_LOAD_OP_LOAD;
			attachmentInfo.storeOp = colorAttachment.Desc.StoreOp;
			attachmentInfo.clearValue = (i < colorClearValues.size())
				? colorClearValues[i]
				: MakeColorClearValue(0.0f, 0.0f, 0.0f, 1.0f);

			if (colorAttachment.ResolveImage)
			{
				colorAttachment.ResolveImage->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
				attachmentInfo.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
				attachmentInfo.resolveImageView = colorAttachment.ResolveImage->GetView();
				attachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}
			else
			{
				attachmentInfo.resolveMode = VK_RESOLVE_MODE_NONE;
				attachmentInfo.resolveImageView = VK_NULL_HANDLE;
				attachmentInfo.resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			}

			colorAttachments[i] = attachmentInfo;
		}

		VkRenderingAttachmentInfo depthAttachmentInfo{};
		VkRenderingAttachmentInfo* depthPtr = nullptr;
		VkRenderingAttachmentInfo* stencilPtr = nullptr;

		if (useDepthAttachment && m_DepthAttachment)
		{
			m_DepthAttachment->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

			depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthAttachmentInfo.imageView = m_DepthAttachment->GetView();
			depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachmentInfo.loadOp = clearDepthAttachment ? m_CreateInfo.DepthAttachment.LoadOp : VK_ATTACHMENT_LOAD_OP_LOAD;
			depthAttachmentInfo.storeOp = m_CreateInfo.DepthAttachment.StoreOp;
			depthAttachmentInfo.clearValue = depthClearValue
				? *depthClearValue
				: MakeDepthClearValue();

			depthPtr = &depthAttachmentInfo;
			if (HasStencilComponent(m_DepthAttachment->GetFormat()))
				stencilPtr = &depthAttachmentInfo;
		}

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.offset = { 0, 0 };
		renderingInfo.renderArea.extent = { m_CreateInfo.Width, m_CreateInfo.Height };
		renderingInfo.layerCount = 1;
		renderingInfo.viewMask = 0;
		renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
		renderingInfo.pColorAttachments = colorAttachments.data();
		renderingInfo.pDepthAttachment = depthPtr;
		renderingInfo.pStencilAttachment = stencilPtr;

		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		m_IsRendering = true;
	}

	void VulkanRenderTarget::EndRendering(
		VkCommandBuffer commandBuffer,
		bool transitionSampledImages,
		bool transitionSampledDepth)
	{
		KITA_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE, "VulkanRenderTarget EndRendering commandBuffer is null");
		KITA_CORE_ASSERT(m_IsRendering, "VulkanRenderTarget EndRendering called without BeginRendering");

		vkCmdEndRendering(commandBuffer);
		m_IsRendering = false;

		if (transitionSampledImages)
		{
			for (ColorAttachment& colorAttachment : m_ColorAttachments)
			{
				VulkanImage& sampledImage = colorAttachment.ResolveImage
					? *colorAttachment.ResolveImage
					: colorAttachment.Image;

				if ((sampledImage.GetUsage() & VK_IMAGE_USAGE_SAMPLED_BIT) != 0)
					sampledImage.TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}

		if (transitionSampledDepth && m_DepthAttachment)
		{
			if ((m_DepthAttachment->GetUsage() & VK_IMAGE_USAGE_SAMPLED_BIT) != 0)
				m_DepthAttachment->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}

	VulkanContext& VulkanRenderTarget::GetContext() const
	{
		KITA_CORE_ASSERT(m_Context, "VulkanRenderTarget context is null");
		return *m_Context;
	}

	const VulkanImage& VulkanRenderTarget::GetColorAttachment(uint32_t index) const
	{
		KITA_CORE_ASSERT(index < m_ColorAttachments.size(), "VulkanRenderTarget color attachment index out of range");
		return m_ColorAttachments[index].Image;
	}

	const VulkanImage* VulkanRenderTarget::GetResolveAttachment(uint32_t index) const
	{
		KITA_CORE_ASSERT(index < m_ColorAttachments.size(), "VulkanRenderTarget resolve attachment index out of range");
		return m_ColorAttachments[index].ResolveImage.get();
	}

	const VulkanImage& VulkanRenderTarget::GetSampledColorAttachment(uint32_t index) const
	{
		KITA_CORE_ASSERT(index < m_ColorAttachments.size(), "VulkanRenderTarget sampled color attachment index out of range");
		const ColorAttachment& attachment = m_ColorAttachments[index];
		return attachment.ResolveImage ? *attachment.ResolveImage : attachment.Image;
	}

	VkFormat VulkanRenderTarget::GetColorFormat(uint32_t index) const
	{
		return GetColorAttachment(index).GetFormat();
	}

	VkFormat VulkanRenderTarget::GetDepthFormat() const
	{
		KITA_CORE_ASSERT(m_DepthAttachment, "VulkanRenderTarget has no depth attachment");
		return m_DepthAttachment->GetFormat();
	}

	VkDescriptorImageInfo VulkanRenderTarget::GetSampledColorDescriptorInfo(
		uint32_t index,
		VkImageLayout samplerLayout) const
	{
		return GetSampledColorAttachment(index).GetDescriptorInfo(samplerLayout);
	}

	VkDescriptorImageInfo VulkanRenderTarget::GetDepthDescriptorInfo(
		VkImageLayout samplerLayout) const
	{
		KITA_CORE_ASSERT(m_DepthAttachment, "VulkanRenderTarget has no depth attachment");
		return m_DepthAttachment->GetDescriptorInfo(samplerLayout);
	}

	VkClearValue VulkanRenderTarget::MakeColorClearValue(float r, float g, float b, float a)
	{
		VkClearValue clearValue{};
		clearValue.color.float32[0] = r;
		clearValue.color.float32[1] = g;
		clearValue.color.float32[2] = b;
		clearValue.color.float32[3] = a;
		return clearValue;
	}

	VkClearValue VulkanRenderTarget::MakeDepthClearValue(float depth, uint32_t stencil)
	{
		VkClearValue clearValue{};
		clearValue.depthStencil.depth = depth;
		clearValue.depthStencil.stencil = stencil;
		return clearValue;
	}

	void VulkanRenderTarget::CreateAttachments()
	{
		m_ColorAttachments.clear();
		m_ColorAttachments.reserve(m_CreateInfo.ColorAttachments.size());

		for (uint32_t i = 0; i < m_CreateInfo.ColorAttachments.size(); ++i)
		{
			m_ColorAttachments.emplace_back();
			CreateColorAttachment(i, m_CreateInfo.ColorAttachments[i]);
		}

		if (m_CreateInfo.DepthAttachment.Enabled)
			CreateDepthAttachment(m_CreateInfo.DepthAttachment);
	}

	void VulkanRenderTarget::CreateColorAttachment(uint32_t index, const ColorAttachmentDesc& desc)
	{
		KITA_CORE_ASSERT(desc.Format != VK_FORMAT_UNDEFINED, "VulkanRenderTarget color attachment format is invalid");

		ColorAttachment& colorAttachment = m_ColorAttachments[index];
		colorAttachment.Desc = desc;

		const bool createResolve = desc.CreateResolveImage && (m_CreateInfo.Samples != VK_SAMPLE_COUNT_1_BIT);

		VulkanImage::CreateInfo imageInfo{};
		imageInfo.Name = desc.Name.empty()
			? MakeAttachmentName(m_CreateInfo.Name, "Color", index)
			: desc.Name;
		imageInfo.Type = VK_IMAGE_TYPE_2D;
		imageInfo.ViewType = VK_IMAGE_VIEW_TYPE_2D;
		imageInfo.Format = desc.Format;
		imageInfo.Extent = { m_CreateInfo.Width, m_CreateInfo.Height, 1 };
		imageInfo.MipLevels = 1;
		imageInfo.ArrayLayers = 1;
		imageInfo.Samples = m_CreateInfo.Samples;
		imageInfo.Tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | desc.ExtraUsage;
		imageInfo.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

		if (m_CreateInfo.Samples == VK_SAMPLE_COUNT_1_BIT && desc.CreateSampler)
		{
			imageInfo.Usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
			imageInfo.CreateSampler = true;
			imageInfo.MinFilter = desc.Filter;
			imageInfo.MagFilter = desc.Filter;
			imageInfo.AddressModeU = desc.AddressMode;
			imageInfo.AddressModeV = desc.AddressMode;
			imageInfo.AddressModeW = desc.AddressMode;
		}

		colorAttachment.Image.Init(*m_Context, imageInfo);

		if (createResolve)
		{
			VulkanImage::CreateInfo resolveInfo{};
			resolveInfo.Name = imageInfo.Name + "_Resolve";
			resolveInfo.Type = VK_IMAGE_TYPE_2D;
			resolveInfo.ViewType = VK_IMAGE_VIEW_TYPE_2D;
			resolveInfo.Format = desc.Format;
			resolveInfo.Extent = { m_CreateInfo.Width, m_CreateInfo.Height, 1 };
			resolveInfo.MipLevels = 1;
			resolveInfo.ArrayLayers = 1;
			resolveInfo.Samples = VK_SAMPLE_COUNT_1_BIT;
			resolveInfo.Tiling = VK_IMAGE_TILING_OPTIMAL;
			resolveInfo.Usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | desc.ExtraUsage;
			resolveInfo.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;

			if (desc.CreateSampler)
			{
				resolveInfo.Usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
				resolveInfo.CreateSampler = true;
				resolveInfo.MinFilter = desc.Filter;
				resolveInfo.MagFilter = desc.Filter;
				resolveInfo.AddressModeU = desc.AddressMode;
				resolveInfo.AddressModeV = desc.AddressMode;
				resolveInfo.AddressModeW = desc.AddressMode;
			}

			colorAttachment.ResolveImage = CreateUnique<VulkanImage>(*m_Context, resolveInfo);
		}
	}

	void VulkanRenderTarget::CreateDepthAttachment(const DepthAttachmentDesc& desc)
	{
		KITA_CORE_ASSERT(desc.Format != VK_FORMAT_UNDEFINED, "VulkanRenderTarget depth attachment format is invalid");

		VulkanImage::CreateInfo imageInfo{};
		imageInfo.Name = desc.Name.empty() ? (m_CreateInfo.Name + "_Depth") : desc.Name;
		imageInfo.Type = VK_IMAGE_TYPE_2D;
		imageInfo.ViewType = VK_IMAGE_VIEW_TYPE_2D;
		imageInfo.Format = desc.Format;
		imageInfo.Extent = { m_CreateInfo.Width, m_CreateInfo.Height, 1 };
		imageInfo.MipLevels = 1;
		imageInfo.ArrayLayers = 1;
		imageInfo.Samples = m_CreateInfo.Samples;
		imageInfo.Tiling = VK_IMAGE_TILING_OPTIMAL;
		imageInfo.Usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | desc.ExtraUsage;
		imageInfo.AspectFlags = 0;

		if (desc.CreateSampler)
		{
			imageInfo.Usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
			imageInfo.CreateSampler = true;
			imageInfo.MinFilter = desc.Filter;
			imageInfo.MagFilter = desc.Filter;
			imageInfo.AddressModeU = desc.AddressMode;
			imageInfo.AddressModeV = desc.AddressMode;
			imageInfo.AddressModeW = desc.AddressMode;
		}

		m_DepthAttachment = CreateUnique<VulkanImage>(*m_Context, imageInfo);
	}

	bool VulkanRenderTarget::HasStencilComponent(VkFormat format)
	{
		switch (format)
		{
		case VK_FORMAT_S8_UINT:
		case VK_FORMAT_D16_UNORM_S8_UINT:
		case VK_FORMAT_D24_UNORM_S8_UINT:
		case VK_FORMAT_D32_SFLOAT_S8_UINT:
			return true;
		default:
			return false;
		}
	}

}
