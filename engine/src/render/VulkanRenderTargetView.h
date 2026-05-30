#pragma once

#include "core/Core.h"
#include "render/VulkanImage.h"

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace Kita {

	class VulkanContext;
	class VulkanRenderTarget;

	// 非拥有型渲染目标视图。
	// 用于把“谁拥有 image”与“本次 pass 如何引用这些 attachment”解耦，
	// 便于 RenderGraph 后续把 transient image 也组装成统一的渲染目标输入。
	class VulkanRenderTargetView
	{
	public:
		struct ColorAttachmentView
		{
			VulkanImage* Image = nullptr;
			VulkanImage* ResolveImage = nullptr;
			VkAttachmentLoadOp LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			VkAttachmentStoreOp StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		};

		struct DepthAttachmentView
		{
			VulkanImage* Image = nullptr;
			VkAttachmentLoadOp LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			VkAttachmentStoreOp StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		};

		struct CreateInfo
		{
			std::string Name;
			VulkanContext* Context = nullptr;
			uint32_t Width = 1;
			uint32_t Height = 1;
			VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;
			std::vector<ColorAttachmentView> ColorAttachments;
			DepthAttachmentView DepthAttachment{};
		};

	public:
		VulkanRenderTargetView() = default;
		explicit VulkanRenderTargetView(const CreateInfo& createInfo);

		void Init(const CreateInfo& createInfo);
		void Reset();

		bool IsValid() const { return m_Context != nullptr && !m_ColorAttachments.empty(); }
		bool HasDepthAttachment() const { return m_DepthAttachment.Image != nullptr; }
		bool IsMultisampled() const { return m_Samples != VK_SAMPLE_COUNT_1_BIT; }

		void BeginRendering(
			VkCommandBuffer commandBuffer,
			const std::vector<VkClearValue>& colorClearValues,
			const VkClearValue* depthClearValue = nullptr,
			bool useDepthAttachment = true,
			bool clearColors = true,
			bool clearDepthAttachment = true);

		void EndRendering(
			VkCommandBuffer commandBuffer,
			bool transitionSampledImages = true,
			bool transitionSampledDepth = false);

		VulkanContext& GetContext() const;
		const std::string& GetName() const { return m_Name; }
		VkExtent2D GetExtent() const { return { m_Width, m_Height }; }
		uint32_t GetWidth() const { return m_Width; }
		uint32_t GetHeight() const { return m_Height; }
		VkSampleCountFlagBits GetSamples() const { return m_Samples; }

		uint32_t GetColorAttachmentCount() const { return static_cast<uint32_t>(m_ColorAttachments.size()); }

		const VulkanImage& GetColorAttachment(uint32_t index) const;
		const VulkanImage* GetResolveAttachment(uint32_t index) const;
		const VulkanImage& GetSampledColorAttachment(uint32_t index) const;
		const VulkanImage* GetDepthAttachment() const { return m_DepthAttachment.Image; }

		VkFormat GetColorFormat(uint32_t index) const;
		VkFormat GetDepthFormat() const;

		VkDescriptorImageInfo GetSampledColorDescriptorInfo(
			uint32_t index,
			VkImageLayout samplerLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const;

		VkDescriptorImageInfo GetDepthDescriptorInfo(
			VkImageLayout samplerLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const;

		static VkClearValue MakeColorClearValue(float r, float g, float b, float a);
		static VkClearValue MakeDepthClearValue(float depth = 1.0f, uint32_t stencil = 0);

	private:
		static bool HasStencilComponent(VkFormat format);

	private:
		VulkanContext* m_Context = nullptr;
		std::string m_Name;
		uint32_t m_Width = 1;
		uint32_t m_Height = 1;
		VkSampleCountFlagBits m_Samples = VK_SAMPLE_COUNT_1_BIT;
		std::vector<ColorAttachmentView> m_ColorAttachments;
		DepthAttachmentView m_DepthAttachment{};
		bool m_IsRendering = false;
	};

	inline VulkanRenderTargetView::VulkanRenderTargetView(const CreateInfo& createInfo)
	{
		Init(createInfo);
	}

	inline void VulkanRenderTargetView::Init(const CreateInfo& createInfo)
	{
		KITA_CORE_ASSERT(createInfo.Context, "VulkanRenderTargetView context is null");
		KITA_CORE_ASSERT(createInfo.Width > 0, "VulkanRenderTargetView width must be > 0");
		KITA_CORE_ASSERT(createInfo.Height > 0, "VulkanRenderTargetView height must be > 0");
		KITA_CORE_ASSERT(!createInfo.ColorAttachments.empty(), "VulkanRenderTargetView requires at least one color attachment");

		m_Context = createInfo.Context;
		m_Name = createInfo.Name;
		m_Width = createInfo.Width;
		m_Height = createInfo.Height;
		m_Samples = createInfo.Samples;
		m_ColorAttachments = createInfo.ColorAttachments;
		m_DepthAttachment = createInfo.DepthAttachment;
		m_IsRendering = false;
	}

	inline void VulkanRenderTargetView::Reset()
	{
		m_Context = nullptr;
		m_Name.clear();
		m_Width = 1;
		m_Height = 1;
		m_Samples = VK_SAMPLE_COUNT_1_BIT;
		m_ColorAttachments.clear();
		m_DepthAttachment = {};
		m_IsRendering = false;
	}

	inline void VulkanRenderTargetView::BeginRendering(
		VkCommandBuffer commandBuffer,
		const std::vector<VkClearValue>& colorClearValues,
		const VkClearValue* depthClearValue,
		bool useDepthAttachment,
		bool clearColors,
		bool clearDepthAttachment)
	{
		KITA_CORE_ASSERT(m_Context, "VulkanRenderTargetView context is null");
		KITA_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE, "VulkanRenderTargetView BeginRendering commandBuffer is null");
		KITA_CORE_ASSERT(!m_IsRendering, "VulkanRenderTargetView is already rendering");

		std::vector<VkRenderingAttachmentInfo> colorAttachments(m_ColorAttachments.size());

		for (size_t i = 0; i < m_ColorAttachments.size(); ++i)
		{
			ColorAttachmentView& colorAttachment = m_ColorAttachments[i];
			KITA_CORE_ASSERT(colorAttachment.Image, "VulkanRenderTargetView color attachment image is null");

			colorAttachment.Image->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

			VkRenderingAttachmentInfo attachmentInfo{};
			attachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			attachmentInfo.imageView = colorAttachment.Image->GetView();
			attachmentInfo.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			attachmentInfo.loadOp = clearColors ? colorAttachment.LoadOp : VK_ATTACHMENT_LOAD_OP_LOAD;
			attachmentInfo.storeOp = colorAttachment.StoreOp;
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

		if (useDepthAttachment && m_DepthAttachment.Image)
		{
			m_DepthAttachment.Image->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

			depthAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			depthAttachmentInfo.imageView = m_DepthAttachment.Image->GetView();
			depthAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthAttachmentInfo.loadOp = clearDepthAttachment ? m_DepthAttachment.LoadOp : VK_ATTACHMENT_LOAD_OP_LOAD;
			depthAttachmentInfo.storeOp = m_DepthAttachment.StoreOp;
			depthAttachmentInfo.clearValue = depthClearValue ? *depthClearValue : MakeDepthClearValue();

			depthPtr = &depthAttachmentInfo;
			if (HasStencilComponent(m_DepthAttachment.Image->GetFormat()))
				stencilPtr = &depthAttachmentInfo;
		}

		VkRenderingInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		renderingInfo.renderArea.offset = { 0, 0 };
		renderingInfo.renderArea.extent = { m_Width, m_Height };
		renderingInfo.layerCount = 1;
		renderingInfo.viewMask = 0;
		renderingInfo.colorAttachmentCount = static_cast<uint32_t>(colorAttachments.size());
		renderingInfo.pColorAttachments = colorAttachments.data();
		renderingInfo.pDepthAttachment = depthPtr;
		renderingInfo.pStencilAttachment = stencilPtr;

		vkCmdBeginRendering(commandBuffer, &renderingInfo);
		m_IsRendering = true;
	}

	inline void VulkanRenderTargetView::EndRendering(
		VkCommandBuffer commandBuffer,
		bool transitionSampledImages,
		bool transitionSampledDepth)
	{
		KITA_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE, "VulkanRenderTargetView EndRendering commandBuffer is null");
		KITA_CORE_ASSERT(m_IsRendering, "VulkanRenderTargetView EndRendering called without BeginRendering");

		vkCmdEndRendering(commandBuffer);
		m_IsRendering = false;

		if (transitionSampledImages)
		{
			for (ColorAttachmentView& colorAttachment : m_ColorAttachments)
			{
				KITA_CORE_ASSERT(colorAttachment.Image, "VulkanRenderTargetView color attachment image is null");

				VulkanImage& sampledImage = colorAttachment.ResolveImage
					? *colorAttachment.ResolveImage
					: *colorAttachment.Image;

				if ((sampledImage.GetUsage() & VK_IMAGE_USAGE_SAMPLED_BIT) != 0)
					sampledImage.TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
			}
		}

		if (transitionSampledDepth && m_DepthAttachment.Image)
		{
			if ((m_DepthAttachment.Image->GetUsage() & VK_IMAGE_USAGE_SAMPLED_BIT) != 0)
				m_DepthAttachment.Image->TransitionLayout(commandBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		}
	}

	inline VulkanContext& VulkanRenderTargetView::GetContext() const
	{
		KITA_CORE_ASSERT(m_Context, "VulkanRenderTargetView context is null");
		return *m_Context;
	}

	inline const VulkanImage& VulkanRenderTargetView::GetColorAttachment(uint32_t index) const
	{
		KITA_CORE_ASSERT(index < m_ColorAttachments.size(), "VulkanRenderTargetView color attachment index out of range");
		KITA_CORE_ASSERT(m_ColorAttachments[index].Image, "VulkanRenderTargetView color attachment image is null");
		return *m_ColorAttachments[index].Image;
	}

	inline const VulkanImage* VulkanRenderTargetView::GetResolveAttachment(uint32_t index) const
	{
		KITA_CORE_ASSERT(index < m_ColorAttachments.size(), "VulkanRenderTargetView resolve attachment index out of range");
		return m_ColorAttachments[index].ResolveImage;
	}

	inline const VulkanImage& VulkanRenderTargetView::GetSampledColorAttachment(uint32_t index) const
	{
		KITA_CORE_ASSERT(index < m_ColorAttachments.size(), "VulkanRenderTargetView sampled color attachment index out of range");
		const ColorAttachmentView& attachment = m_ColorAttachments[index];
		KITA_CORE_ASSERT(attachment.Image, "VulkanRenderTargetView sampled color attachment image is null");
		return attachment.ResolveImage ? *attachment.ResolveImage : *attachment.Image;
	}

	inline VkFormat VulkanRenderTargetView::GetColorFormat(uint32_t index) const
	{
		return GetColorAttachment(index).GetFormat();
	}

	inline VkFormat VulkanRenderTargetView::GetDepthFormat() const
	{
		KITA_CORE_ASSERT(m_DepthAttachment.Image, "VulkanRenderTargetView has no depth attachment");
		return m_DepthAttachment.Image->GetFormat();
	}

	inline VkDescriptorImageInfo VulkanRenderTargetView::GetSampledColorDescriptorInfo(
		uint32_t index,
		VkImageLayout samplerLayout) const
	{
		return GetSampledColorAttachment(index).GetDescriptorInfo(samplerLayout);
	}

	inline VkDescriptorImageInfo VulkanRenderTargetView::GetDepthDescriptorInfo(
		VkImageLayout samplerLayout) const
	{
		KITA_CORE_ASSERT(m_DepthAttachment.Image, "VulkanRenderTargetView has no depth attachment");
		return m_DepthAttachment.Image->GetDescriptorInfo(samplerLayout);
	}

	inline VkClearValue VulkanRenderTargetView::MakeColorClearValue(float r, float g, float b, float a)
	{
		VkClearValue clearValue{};
		clearValue.color.float32[0] = r;
		clearValue.color.float32[1] = g;
		clearValue.color.float32[2] = b;
		clearValue.color.float32[3] = a;
		return clearValue;
	}

	inline VkClearValue VulkanRenderTargetView::MakeDepthClearValue(float depth, uint32_t stencil)
	{
		VkClearValue clearValue{};
		clearValue.depthStencil.depth = depth;
		clearValue.depthStencil.stencil = stencil;
		return clearValue;
	}

	inline bool VulkanRenderTargetView::HasStencilComponent(VkFormat format)
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
