#pragma once

#include "Asset.h"
#include "render/material/MaterialRuntimeLayout.h"
#include "render/shaderLab/ShaderLabCompiler.h"
#include "render/shaderLab/ShaderLabType.h"

#include <memory>

namespace Kita {

	struct ShaderLabAsset : public Asset
	{
		virtual AssetType GetType() const override { return AssetType::ShaderLab; }

		std::filesystem::path SourcePath;

		// 解析后的静态 ShaderLab 描述。
		std::shared_ptr<ShaderLabAssetDesc> Desc = nullptr;

		// 编译后的材质运行时布局。
		std::shared_ptr<MaterialRuntimeLayout> RuntimeLayout = nullptr;

		// 缓存统一 DeferredLighting 所需的材质级 lighting 元数据。
		std::shared_ptr<ShaderLabLightingRuntimeDesc> LightingRuntime = nullptr;

		// 每个 Pass 编译出的运行时 shader binary。
		std::vector<ShaderLabCompiledPass> CompiledPasses;
	};

}
