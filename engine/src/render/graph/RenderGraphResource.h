#pragma once
#include <string>
#include <cstdint>


namespace Kita {

	class VulkanRenderTarget;

	using RenderGraphResourceID = uint32_t;
	static constexpr RenderGraphResourceID InvalidRenderGraphResourceID = UINT32_MAX;

	enum class RenderGraphResourceType
	{
		UnKown = 0,
		ImportedRenderTarget
	};

	struct RenderGraphResource
	{
		std::string Name;
		RenderGraphResourceType Type = RenderGraphResourceType::UnKown;
		VulkanRenderTarget* RT = nullptr;
	};



}