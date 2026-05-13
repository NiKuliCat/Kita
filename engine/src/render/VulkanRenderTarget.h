#pragma once

#include "core/Core.h"
#include "render/VulkanImage.h"

#include <vulkan/vulkan.h>
#include <string>
#include <vector>

namespace Kita {

	class VulkanContext;

	class VulkanRenderTarget
	{
	public:
		struct ColorAttachmentDesc
		{
			std::string Name;
			VkFormat Format = VK_FORMAT_UNDEFINED;

			VkImageUsageFlags ExtraUsage = 0;

			bool CreateSampler = true;
			bool CreateResolveImage = false;

			VkFilter Filter = VK_FILTER_LINEAR;
			VkSamplerAddressMode AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

			VkAttachmentLoadOp LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			VkAttachmentStoreOp StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		};

		struct DepthAttachmentDesc
		{
			bool Enabled = false;
			std::string Name;
			VkFormat Format = VK_FORMAT_D32_SFLOAT;

			VkImageUsageFlags ExtraUsage = 0;

			bool CreateSampler = false;
			VkFilter Filter = VK_FILTER_LINEAR;
			VkSamplerAddressMode AddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;

			VkAttachmentLoadOp LoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			VkAttachmentStoreOp StoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		};

		struct CreateInfo
		{
			std::string Name;

			uint32_t Width = 1;
			uint32_t Height = 1;
			VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;

			std::vector<ColorAttachmentDesc> ColorAttachments;
			DepthAttachmentDesc DepthAttachment{};
		};

	private:
		struct ColorAttachment
		{
			ColorAttachmentDesc Desc{};
			VulkanImage Image;
			Unique<VulkanImage> ResolveImage = nullptr;
		};

	public:
		VulkanRenderTarget() = default;
		VulkanRenderTarget(VulkanContext& context, const CreateInfo& createInfo);
		~VulkanRenderTarget();

		VulkanRenderTarget(const VulkanRenderTarget&) = delete;
		VulkanRenderTarget& operator=(const VulkanRenderTarget&) = delete;

		VulkanRenderTarget(VulkanRenderTarget&& other) noexcept;
		VulkanRenderTarget& operator=(VulkanRenderTarget&& other) noexcept;

		void Init(VulkanContext& context, const CreateInfo& createInfo);
		void Destroy();
		void Resize(uint32_t width, uint32_t height);

		bool IsValid() const { return m_Context != nullptr && !m_ColorAttachments.empty(); }
		bool HasDepthAttachment() const { return m_DepthAttachment != nullptr; }
		bool IsMultisampled() const { return m_CreateInfo.Samples != VK_SAMPLE_COUNT_1_BIT; }

		void BeginRendering(
			VkCommandBuffer commandBuffer,
			const std::vector<VkClearValue>& colorClearValues,
			const VkClearValue* depthClearValue = nullptr);

		void EndRendering(
			VkCommandBuffer commandBuffer,
			bool transitionSampledImages = true,
			bool transitionSampledDepth = false);

		VulkanContext& GetContext() const;

		const CreateInfo& GetCreateInfo() const { return m_CreateInfo; }
		VkExtent2D GetExtent() const { return { m_CreateInfo.Width, m_CreateInfo.Height }; }
		uint32_t GetWidth() const { return m_CreateInfo.Width; }
		uint32_t GetHeight() const { return m_CreateInfo.Height; }

		uint32_t GetColorAttachmentCount() const { return static_cast<uint32_t>(m_ColorAttachments.size()); }

		const VulkanImage& GetColorAttachment(uint32_t index) const;
		const VulkanImage* GetResolveAttachment(uint32_t index) const;
		const VulkanImage& GetSampledColorAttachment(uint32_t index) const;

		const VulkanImage* GetDepthAttachment() const { return m_DepthAttachment.get(); }

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
		void CreateAttachments();
		void CreateColorAttachment(uint32_t index, const ColorAttachmentDesc& desc);
		void CreateDepthAttachment(const DepthAttachmentDesc& desc);

		static bool HasStencilComponent(VkFormat format);

	private:
		VulkanContext* m_Context = nullptr;
		CreateInfo m_CreateInfo{};
		std::vector<ColorAttachment> m_ColorAttachments;
		Unique<VulkanImage> m_DepthAttachment = nullptr;
		bool m_IsRendering = false;
	};

}
