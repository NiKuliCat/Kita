#pragma once

#include "RenderGraphResource.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace Kita {

	static constexpr uint32_t InvalidRenderGraphPassIndex = std::numeric_limits<uint32_t>::max();

	struct RenderGraphCompiledPass
	{
		uint32_t PassIndex = InvalidRenderGraphPassIndex;

		// 带访问语义的资源读写声明。
		// 后续 layout/barrier、frame debugger、RT debug 都从这里读取资源、附件和访问类型。
		std::vector<RenderGraphResourceAccess> ReadAccesses;
		std::vector<RenderGraphResourceAccess> WriteAccesses;
	};

	// RenderGraph 资源生命周期。
	// 后续 transient allocator 会根据这些信息决定资源创建、释放与复用时机。
	struct RenderGraphResourceLifetime
	{
		uint32_t FirstWritePass = InvalidRenderGraphPassIndex;
		uint32_t LastReadPass = InvalidRenderGraphPassIndex;
		uint32_t LastAccessPass = InvalidRenderGraphPassIndex;
	};

	// RenderGraph 编译后得到的单个资源访问汇总。
	// allocator 用 Usage 创建 transient image，layout/barrier 阶段会继续扩展这里。
	struct RenderGraphResourceAccessSummary
	{
		VkImageUsageFlags Usage = 0;

		VkImageLayout FirstLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		VkImageLayout LastLayout = VK_IMAGE_LAYOUT_UNDEFINED;

		bool HasRead = false;
		bool HasWrite = false;
	};

	// RenderGraph 编译结果。
	// 后续拓扑排序、barrier、pass culling、资源复用都从这里扩展。
	struct RenderGraphCompileResult
	{
		bool Valid = false;
		std::vector<RenderGraphCompiledPass> Passes;

		// 下标与 RenderGraphResourceID 对齐。
		std::vector<RenderGraphResourceLifetime> ResourceLifetimes;

		// 下标与 RenderGraphResourceID 对齐，记录每个资源在本次 graph 中的 Vulkan 使用需求。
		std::vector<RenderGraphResourceAccessSummary> ResourceAccessSummaries;
	};

}
