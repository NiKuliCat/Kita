#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "asset/Asset.h"
#include "render/ShaderCompiler.h"
#include "render/material/MaterialRuntimeLayout.h"
#include "ShaderLabType.h"

namespace Kita {

	struct MaterialCompiledPass
	{
		std::string Name;
		PassType Type = PassType::Unknown;

		MaterialRenderStateDesc RenderState;
		MaterialRenderGraphDesc RenderGraph;
		MaterialProgramDesc Program;

		ShaderStageBinary VertexStage;
		ShaderStageBinary FragmentStage;

		// 方便排查编译问题时输出包裹后的最终源码。
		std::string WrappedSource;
	};

	struct MaterialCompileResult
	{
		bool Success = false;

		MaterialDefinitionDesc SourceAsset;
		MaterialRuntimeLayout MaterialLayout;
		MaterialLightingRuntimeDesc LightingRuntime;
		std::vector<MaterialCompiledPass> Passes;

		std::string Diagnostics;
	};

	class MaterialCompiler
	{
	public:
		MaterialCompiler() = default;

		MaterialCompileResult CompileAsset(
			const MaterialDefinitionDesc& assetDesc,
			const std::filesystem::path& materialPath) const;

		static MaterialRuntimeLayout BuildMaterialLayout(
			const std::vector<MaterialPropertyDesc>& properties,
			bool includeSystemFields);

		static MaterialLightingRuntimeDesc BuildLightingRuntime(
			const MaterialLightingDesc& lightingDesc);

		static std::string GenerateWrappedSource(
			const MaterialDefinitionDesc& assetDesc,
			const MaterialPassDesc& passDesc,
			const MaterialRuntimeLayout& materialLayout,
			const std::string& userSource);

	private:
		static ShaderCompiler::CompileRequest BuildCompileRequest(
			const MaterialDefinitionDesc& assetDesc,
			const MaterialPassDesc& passDesc,
			const std::filesystem::path& materialPath,
			ShaderCompiler::Stage stage);

		static std::string BuildMaterialPrelude(
			const std::vector<MaterialPropertyDesc>& properties,
			const MaterialRuntimeLayout& materialLayout);

		static std::string BuildLightingPrelude(
			const MaterialLightingDesc& lightingDesc);

		static std::string ReadUserSource(const std::filesystem::path& path);
	};

	using ShaderLabCompiledPass = MaterialCompiledPass;
	using ShaderLabCompileResult = MaterialCompileResult;
	using ShaderLabCompiler = MaterialCompiler;

}
