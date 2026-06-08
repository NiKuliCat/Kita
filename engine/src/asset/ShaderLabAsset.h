#pragma once

#include "Asset.h"
#include "render/material/MaterialRuntimeLayout.h"
#include "render/shaderLab/ShaderLabCompiler.h"
#include "render/shaderLab/ShaderLabType.h"

#include <memory>

namespace Kita {

	struct MaterialDefinitionAsset : public Asset
	{
		virtual AssetType GetType() const override { return AssetType::MaterialDefinition; }

		std::filesystem::path SourcePath;

		// 保存解析后的母材质定义文本描述。
		std::shared_ptr<MaterialDefinitionDesc> Desc = nullptr;

		// 保存编译后的运行时参数布局。
		std::shared_ptr<MaterialRuntimeLayout> RuntimeLayout = nullptr;

		// 缓存 Surface 材质参与 deferred lighting 所需的元信息。
		std::shared_ptr<MaterialLightingRuntimeDesc> LightingRuntime = nullptr;

		// 每个 Pass 编译出的运行时 shader binary。
		std::vector<MaterialCompiledPass> CompiledPasses;
	};

	using ShaderLabAsset = MaterialDefinitionAsset;

}
