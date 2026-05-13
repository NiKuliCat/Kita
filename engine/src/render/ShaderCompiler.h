#pragma once

#include "core/Core.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <slang/slang.h>
#include <slang/slang-com-ptr.h>
#include <slang/slang-com-helper.h>

namespace Kita {

	class ShaderCompiler
	{
	public:
		enum class Stage
		{
			Vertex,
			Fragment,
			Compute,
			Geometry,
			TessControl,
			TessEvaluation
		};

		struct CompileRequest
		{
			std::filesystem::path SourcePath;
			std::string ModuleName = "shader";
			std::string EntryPointName = "main";
			Stage ShaderStage = Stage::Vertex;
			std::string Profile = "spirv_1_5";
			std::vector<std::filesystem::path> IncludeDirs;
			std::unordered_map<std::string, std::string> Defines;
			bool EmitDebugInfo = true;
			bool Optimize = false;
		};

		struct CompileResult
		{
			bool Success = false;
			std::string Diagnostics;
			std::vector<uint8_t> Spirv;
		};

	public:
		ShaderCompiler();
		~ShaderCompiler() = default;

		CompileResult CompileToSpirv(const CompileRequest& request);
		CompileResult CompileToSpirvFromSource(const CompileRequest& request, const std::string& source);

		static std::string ReadTextFile(const std::filesystem::path& path);
		static bool WriteBinaryFile(const std::filesystem::path& path, const void* data, size_t size);

	private:
		void EnsureGlobalSession();
		CompileResult CompileToSpirvInternal(const CompileRequest& request, const std::string& source);
		static std::string GetDiagnosticsString(slang::IBlob* diagnostics);
		static void LogDiagnostics(slang::IBlob* diagnostics);
		static void AppendMacro(std::vector<slang::PreprocessorMacroDesc>& macros, const std::string& name, const std::string& value);
		static SlangStage  ToSlangStage(Stage stage);

	private:
		Slang::ComPtr<slang::IGlobalSession> m_GlobalSession;
	};

}
