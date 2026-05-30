#pragma once
#include <string>
#include <cstdint>
#include <vulkan/vulkan.h>

namespace Kita {

	class VulkanRenderTarget;

	using RenderGraphResourceID = uint32_t;
	static constexpr RenderGraphResourceID InvalidRenderGraphResourceID = UINT32_MAX;

	enum class RenderGraphResourceType
	{
		Unkown = 0,
		ImportedRenderTarget,
		TransientTexture
	};

	// RenderGraph 中的逻辑纹理描述。
	// 当前阶段只记录资源需求，不负责创建 VkImage。
	struct RenderGraphTextureDesc
	{
		std::string Name;
		uint32_t Width = 1;
		uint32_t Height = 1;
		VkFormat Format = VK_FORMAT_UNDEFINED;
		VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;

		VkImageUsageFlags Usage = 0;
		bool CreateSampler = false;
	};

	struct RenderGraphResource
	{
		std::string Name;
		RenderGraphResourceType Type = RenderGraphResourceType::Unkown;

		// 外部导入资源由调用方持有，例如 EditorViewportSurface 创建的 RenderTarget。
		VulkanRenderTarget* RT = nullptr;
		bool Imported = false;


		// TransientTexture 使用该描述，当前阶段不会创建真实 Vulkan 资源。
		RenderGraphTextureDesc TextureDesc{};

	};



}