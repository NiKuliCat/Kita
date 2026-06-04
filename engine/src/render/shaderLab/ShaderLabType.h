#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

#include "asset/Asset.h"
#include "render/pass/RenderDataStruct.h"


namespace Kita {

	// ShaderLab Pass 的固定渲染状态描述。
		// 第二步先只覆盖当前引擎已经用到的状态项，后续如需 Stencil/BlendOp 再扩展。
	struct ShaderLabRenderStateDesc
	{
		VkCullModeFlags CullMode = VK_CULL_MODE_BACK_BIT;
		bool DepthTest = true;
		bool DepthWrite = true;
		VkCompareOp DepthCompareOp = VK_COMPARE_OP_LESS;
		bool Blend = false;
	};

	struct ShaderLabColorOutputDesc
	{
		std::string Name;
		VkFormat Format = VK_FORMAT_UNDEFINED;
	};

	// RenderGraph 描述先只做“材质 Pass 与渲染目标布局”的静态声明，
	// 不直接参与第二步运行时绑定。
	struct ShaderLabRenderGraphDesc
	{
		std::string InputLayout;
		std::string OutputLayout;

		std::vector<ShaderLabColorOutputDesc> Colors;
		bool HasDepth = false;
		std::string DepthName;
		VkFormat DepthFormat = VK_FORMAT_UNDEFINED;
	};

	struct ShaderLabProgramDesc
	{
		std::filesystem::path Source;
		std::string VertexEntry = "VSMain";
		std::string FragmentEntry = "PSMain";
	};

	inline constexpr uint32_t ShaderLabMaxCustomDataCount = 4;

	// Lighting 元数据描述表面材质如何参与统一的延迟光照框架。
	struct ShaderLabLightingDesc
	{
		bool Enabled = false;
		std::string ShadingModel = "DefaultLit";
		uint32_t CustomDataCount = 0;
		std::filesystem::path Include;
		std::string Evaluate;
	};

	// 运行时缓存的 lighting 信息，供 GBuffer 编码和 Uber lighting 分发共用。
	struct ShaderLabLightingRuntimeDesc
	{
		bool Enabled = false;
		std::string ShadingModelName = "DefaultLit";
		uint32_t ShadingModelID = 0;
		uint32_t CustomDataCount = 0;
		std::filesystem::path LightingHookInclude;
		std::string LightingHookFunction;
	};

	struct ShaderLabPassDesc
	{
		std::string Name;
		std::string LightMode;
		PassType Type = PassType::Unknown;

		std::unordered_map<std::string, std::string> Tags;
		ShaderLabRenderStateDesc RenderState;
		ShaderLabRenderGraphDesc RenderGraph;
		ShaderLabProgramDesc Program;
	};

	// ShaderLab 资产的纯描述数据。
	// 第二步只负责把 .shader 解析成这个结构，还不做编译与反射。
	struct ShaderLabAssetDesc
	{
		std::string ShaderName;
		std::vector<MaterialPropertyDesc> Properties;
		ShaderLabLightingDesc Lighting;
		std::unordered_map<std::string, std::string> Tags;
		std::vector<ShaderLabPassDesc> Passes;
	};

}
