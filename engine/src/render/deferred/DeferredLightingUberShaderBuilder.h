#pragma once

#include "asset/AssetManager.h"
#include "render/VulkanResourceFactory.h"

namespace Kita {

	struct DeferredLightingUberShaderBuildResult
	{
		bool Success = false;
		VulkanResourceFactory::ShaderBundle ShaderBundle{};
		std::string Diagnostics;
		std::string GeneratedSource;
		uint32_t SurfaceShaderCount = 0;
		uint32_t CustomShadingModelCount = 0;
	};

	// 统一收集项目里的 ShaderLab Lighting 元数据，生成并编译单张 deferred lighting UberShader。
	class DeferredLightingUberShaderBuilder
	{
	public:
		static DeferredLightingUberShaderBuildResult BuildForProject(
			AssetManager& assetManager,
			VulkanResourceFactory& resourceFactory);
	};

}
