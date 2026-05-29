#pragma once

#include "RenderGraphResource.h"

#include <cstdint>
#include <limits>
#include <vector>

namespace Kita {

	static constexpr uint32_t InvalidRenderGraphPassIndex = std::numeric_limits<uint32_t>::max();

	// 编译后的 RenderGraph Pass。第一阶段只保存稳定的执行索引和资源读写声明。
	struct RenderGraphCompiledPass
	{
		uint32_t PassIndex = InvalidRenderGraphPassIndex;
		std::vector<RenderGraphResourceID> Reads;
		std::vector<RenderGraphResourceID> Writes;
	};

	// RenderGraph 编译结果。后续拓扑排序、barrier、pass culling 都从这里扩展。
	struct RenderGraphCompileResult
	{
		bool Valid = false;
		std::vector<RenderGraphCompiledPass> Passes;
	};

}
