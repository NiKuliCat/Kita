#pragma once
#include "render/ShaderCompiler.h"
namespace Kita{


	class ShaderCompileCache
	{
	public:
		static void SetCacheRoot(const std::filesystem::path& root);

		static bool Load(const  ShaderCompiler::CompileRequest& request, std::vector<uint8_t>& outSpirv);
		static bool Store(const  ShaderCompiler::CompileRequest& request, const std::vector<uint8_t>& spirv);
		static void ClearAll();

	private:
		static std::filesystem::path BuildSpirvPath(const std::string& key);
		static std::filesystem::path BuildManifestPath(const std::string& key);
		static std::string BuildCacheKey(const ShaderCompiler::CompileRequest& request);

	private:
		static std::filesystem::path s_CacheRoot;
	};



}
