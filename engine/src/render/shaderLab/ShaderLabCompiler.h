#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "asset/Asset.h"
#include "render/ShaderCompiler.h"
#include "render/material/MaterialRuntimeLayout.h"
#include "ShaderLabType.h"

namespace Kita {

	struct ShaderLabCompiledPass
	{
		std::string Name;
		PassType Type = PassType::Unknown;

		ShaderLabRenderStateDesc RenderState;
		ShaderLabRenderGraphDesc RenderGraph;
		ShaderLabProgramDesc Program;

		ShaderStageBinary VertexStage;
		ShaderStageBinary FragmentStage;

		// 方便排查编译问题时落日志或导出到临时文件。
		std::string WrappedSource;
	};

	struct ShaderLabCompileResult
	{
		bool Success = false;

		ShaderLabAssetDesc SourceAsset;
		MaterialRuntimeLayout MaterialLayout;
		ShaderLabLightingRuntimeDesc LightingRuntime;
		std::vector<ShaderLabCompiledPass> Passes;

		std::string Diagnostics;
	};

	// ShaderLab 编译器：
	// 1. 根据 Properties 生成统一的材质 UBO/纹理声明
	// 2. 将用户 Slang 源码包装成可直接编译的完整源码
	// 3. 为每个 Pass 编译 VS/FS
	class ShaderLabCompiler
	{
	public:
		ShaderLabCompiler() = default;

		ShaderLabCompileResult CompileAsset(
			const ShaderLabAssetDesc& assetDesc,
			const std::filesystem::path& shaderLabPath) const;

		static MaterialRuntimeLayout BuildMaterialLayout(
			const std::vector<MaterialPropertyDesc>& properties,
			bool includeSystemFields);

		static ShaderLabLightingRuntimeDesc BuildLightingRuntime(
			const ShaderLabLightingDesc& lightingDesc);

		static std::string GenerateWrappedSource(
			const ShaderLabAssetDesc& assetDesc,
			const ShaderLabPassDesc& passDesc,
			const MaterialRuntimeLayout& materialLayout,
			const std::string& userSource);

	private:
		static ShaderCompiler::CompileRequest BuildCompileRequest(
			const ShaderLabAssetDesc& assetDesc,
			const ShaderLabPassDesc& passDesc,
			const std::filesystem::path& shaderLabPath,
			ShaderCompiler::Stage stage);

		static std::string BuildMaterialPrelude(
			const std::vector<MaterialPropertyDesc>& properties,
			const MaterialRuntimeLayout& materialLayout);

		static std::string BuildLightingPrelude(
			const ShaderLabLightingDesc& lightingDesc);

		static std::string ReadUserSource(const std::filesystem::path& path);
	};

}
