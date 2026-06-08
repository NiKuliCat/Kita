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

	enum class MaterialDomain : uint8_t
	{
		Surface = 0,
		PostProcess,
		Utility
	};

	inline const char* MaterialDomainToString(MaterialDomain domain)
	{
		switch (domain)
		{
		case MaterialDomain::Surface: return "Surface";
		case MaterialDomain::PostProcess: return "PostProcess";
		case MaterialDomain::Utility: return "Utility";
		default: return "Surface";
		}
	}

	struct MaterialRenderStateDesc
	{
		VkCullModeFlags CullMode = VK_CULL_MODE_BACK_BIT;
		bool DepthTest = true;
		bool DepthWrite = true;
		VkCompareOp DepthCompareOp = VK_COMPARE_OP_LESS;
		bool Blend = false;
	};

	struct MaterialColorOutputDesc
	{
		std::string Name;
		VkFormat Format = VK_FORMAT_UNDEFINED;
	};

	struct MaterialRenderGraphDesc
	{
		std::string InputLayout;
		std::string OutputLayout;

		std::vector<MaterialColorOutputDesc> Colors;
		bool HasDepth = false;
		std::string DepthName;
		VkFormat DepthFormat = VK_FORMAT_UNDEFINED;
	};

	struct MaterialProgramDesc
	{
		std::filesystem::path Source;
		std::string VertexEntry = "VSMain";
		std::string FragmentEntry = "PSMain";
	};

	inline constexpr uint32_t MaterialMaxCustomDataCount = 4;

	struct MaterialLightingDesc
	{
		bool Enabled = false;
		std::string ShadingModel = "DefaultLit";
		uint32_t CustomDataCount = 0;
		std::filesystem::path Include;
		std::string Evaluate;
	};

	struct MaterialLightingRuntimeDesc
	{
		bool Enabled = false;
		std::string ShadingModelName = "DefaultLit";
		uint32_t ShadingModelID = 0;
		uint32_t CustomDataCount = 0;
		std::filesystem::path LightingHookInclude;
		std::string LightingHookFunction;
	};

	struct MaterialPassDesc
	{
		std::string Name;
		std::string LightMode;
		PassType Type = PassType::Unknown;

		std::unordered_map<std::string, std::string> Tags;
		MaterialRenderStateDesc RenderState;
		MaterialRenderGraphDesc RenderGraph;
		MaterialProgramDesc Program;
	};

	struct MaterialDefinitionDesc
	{
		std::string MaterialName;
		MaterialDomain Domain = MaterialDomain::Surface;
		bool UsesLegacyShaderKeyword = false;
		std::vector<MaterialPropertyDesc> Properties;
		MaterialLightingDesc Lighting;
		std::unordered_map<std::string, std::string> Tags;
		std::vector<MaterialPassDesc> Passes;
	};

	using ShaderLabRenderStateDesc = MaterialRenderStateDesc;
	using ShaderLabColorOutputDesc = MaterialColorOutputDesc;
	using ShaderLabRenderGraphDesc = MaterialRenderGraphDesc;
	using ShaderLabProgramDesc = MaterialProgramDesc;
	using ShaderLabLightingDesc = MaterialLightingDesc;
	using ShaderLabLightingRuntimeDesc = MaterialLightingRuntimeDesc;
	using ShaderLabPassDesc = MaterialPassDesc;
	using ShaderLabAssetDesc = MaterialDefinitionDesc;
	inline constexpr uint32_t ShaderLabMaxCustomDataCount = MaterialMaxCustomDataCount;

}
