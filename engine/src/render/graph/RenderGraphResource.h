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
		Unknown = 0,
		ImportedRenderTarget,
		TransientTexture
	};

	enum class RenderGraphAttachmentType
	{
		Color = 0,
		Depth
	};

	// RenderGraph 中的 attachment/view 级引用。
	// 用于精确描述某个 pass 访问的是资源里的哪一个颜色附件，或者 depth 附件。
	struct RenderGraphAttachmentRef
	{
		RenderGraphResourceID Resource = InvalidRenderGraphResourceID;
		RenderGraphAttachmentType Type = RenderGraphAttachmentType::Color;
		uint32_t Index = 0;

		bool IsValid() const { return Resource != InvalidRenderGraphResourceID; }

		static RenderGraphAttachmentRef MakeColor(
			RenderGraphResourceID resource,
			uint32_t attachmentIndex = 0)
		{
			RenderGraphAttachmentRef ref{};
			ref.Resource = resource;
			ref.Type = RenderGraphAttachmentType::Color;
			ref.Index = attachmentIndex;
			return ref;
		}

		static RenderGraphAttachmentRef MakeDepth(RenderGraphResourceID resource)
		{
			RenderGraphAttachmentRef ref{};
			ref.Resource = resource;
			ref.Type = RenderGraphAttachmentType::Depth;
			ref.Index = 0;
			return ref;
		}
	};

	enum class RenderGraphAccessType
	{
		Unknown = 0,

		// 作为 shader sampled texture 读取。
		SampledTexture,

		// 作为 storage image 读写，后续 compute / image load-store pass 使用。
		StorageImage,

		// 作为 color attachment 写入。
		ColorAttachment,

		// 作为只读 depth/stencil attachment 或 sampled depth 读取。
		DepthRead,

		// 作为 depth/stencil attachment 写入。
		DepthStencilAttachment,

		// 作为拷贝源读取。
		TransferSrc,

		// 作为拷贝目标写入。
		TransferDst,

		// 后续 swapchain / present 相关阶段使用。
		Present
	};

	struct RenderGraphResourceAccess
	{
		RenderGraphAttachmentRef Attachment{};
		RenderGraphAccessType Type = RenderGraphAccessType::Unknown;

		RenderGraphResourceID GetResource() const { return Attachment.Resource; }

	};

	// 根据 RenderGraph 访问语义推导 Vulkan image usage。
	// allocator 创建 transient texture 时会合并所有访问产生的 usage。
	VkImageUsageFlags GetImageUsageForAccess(RenderGraphAccessType type);

	// 根据 RenderGraph 访问语义推导推荐 image layout。
	// 后续 barrier/layout plan 会使用这里作为基础映射。
	VkImageLayout GetImageLayoutForAccess(RenderGraphAccessType type);

	// 判断该访问是否属于写入类访问。
	// 后续 frame debugger、barrier、pass culling 可用它区分输入和输出。
	bool IsWriteAccess(RenderGraphAccessType type);



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
		RenderGraphResourceType Type = RenderGraphResourceType::Unknown;

		// 外部导入资源由调用方持有，例如 EditorViewportSurface 创建的 RenderTarget。
		VulkanRenderTarget* RT = nullptr;
		bool Imported = false;


		// TransientTexture 使用该描述，当前阶段不会创建真实 Vulkan 资源。
		RenderGraphTextureDesc TextureDesc{};

	};



}
