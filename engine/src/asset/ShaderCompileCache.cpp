#include "kita_pch.h"
#include "ShaderCompileCache.h"
#include "core/Log.h"
#include <nlohmann/json.hpp>
namespace Kita {


	std::filesystem::path ShaderCompileCache::s_CacheRoot{};

	namespace
	{
		constexpr uint32_t kShaderCacheVersion = 1;
		constexpr uint64_t kFnvOffsetBasis = 14695981039346656037ull;
		constexpr uint64_t kFnvPrime = 1099511628211ull;

		void HashBytes(uint64_t& hash, const void* data, size_t size)
		{
			const uint8_t* bytes = static_cast<const uint8_t*>(data);
			for (size_t i = 0; i < size; ++i)
			{
				hash ^= bytes[i];
				hash *= kFnvPrime;
			}
		}

		void HashString(uint64_t& hash, const std::string& value)
		{
			HashBytes(hash, value.data(), value.size());

			constexpr char separator = '\0';
			HashBytes(hash, &separator, sizeof(separator));
		}

		void HashBool(uint64_t& hash, bool value)
		{
			HashString(hash, value ? "true" : "false");
		}

		void HashUInt32(uint64_t& hash, uint32_t value)
		{
			HashString(hash, std::to_string(value));
		}

		std::string ToHex(uint64_t value)
		{
			std::ostringstream stream;
			stream << std::hex << std::setw(16) << std::setfill('0') << value;
			return stream.str();
		}

		std::string ReadFileBytesAsString(const std::filesystem::path& path)
		{
			std::ifstream file(path, std::ios::binary);
			if (!file.is_open())
				return {};

			std::ostringstream stream;
			stream << file.rdbuf();
			return stream.str();
		}

		std::string NormalizeCachePath(const std::filesystem::path& path)
		{
			std::error_code ec;
			const std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(path, ec);
			if (!ec)
				return canonicalPath.lexically_normal().generic_string();

			return path.lexically_normal().generic_string();
		}

		const char* StageToString(ShaderCompiler::Stage stage)
		{
			switch (stage)
			{
			case ShaderCompiler::Stage::Vertex:         return "Vertex";
			case ShaderCompiler::Stage::Fragment:       return "Fragment";
			case ShaderCompiler::Stage::Compute:        return "Compute";
			case ShaderCompiler::Stage::Geometry:       return "Geometry";
			case ShaderCompiler::Stage::TessControl:    return "TessControl";
			case ShaderCompiler::Stage::TessEvaluation: return "TessEvaluation";
			default:                                    return "Unknown";
			}
		}

		bool ReadBinaryFile(const std::filesystem::path& path, std::vector<uint8_t>& outBytes)
		{
			outBytes.clear();

			std::ifstream file(path, std::ios::binary | std::ios::ate);
			if (!file.is_open())
				return false;

			const std::streamsize size = file.tellg();
			if (size <= 0)
				return false;

			file.seekg(0, std::ios::beg);
			outBytes.resize(static_cast<size_t>(size));
			return file.read(reinterpret_cast<char*>(outBytes.data()), size).good();
		}

		bool WriteBinaryFile(const std::filesystem::path& path, const std::vector<uint8_t>& bytes)
		{
			if (bytes.empty())
				return false;

			std::error_code ec;
			std::filesystem::create_directories(path.parent_path(), ec);
			if (ec)
				return false;

			std::ofstream file(path, std::ios::binary);
			if (!file.is_open())
				return false;

			file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
			return file.good();
		}
	}

	void ShaderCompileCache::SetCacheRoot(const std::filesystem::path& root)
	{
		s_CacheRoot = root.lexically_normal();
	}

	bool ShaderCompileCache::Load(const ShaderCompiler::CompileRequest& request, std::vector<uint8_t>& outSpirv)
	{
		if(s_CacheRoot.empty())
			return false;

		const std::string key = BuildCacheKey(request);
		if (key.empty())
			return false;

		const std::filesystem::path spirvPath = BuildSpirvPath(key);
		if (!ReadBinaryFile(spirvPath, outSpirv))
			return false;

		KITA_CORE_INFO(
			"Shader cache hit: {} {}",
			request.SourcePath.string(),
			StageToString(request.ShaderStage));

		return true;
	}

	bool ShaderCompileCache::Store(const ShaderCompiler::CompileRequest& request, const  std::vector<uint8_t>& spirv)
	{
		if (s_CacheRoot.empty() || spirv.empty())
			return false;

		const std::string key = BuildCacheKey(request);
		if (key.empty())
			return false;

		const std::filesystem::path spirvPath = BuildSpirvPath(key);
		if (!WriteBinaryFile(spirvPath, spirv))
		{
			KITA_CORE_WARN("Shader cache store failed: {}", spirvPath.string());
			return false;
		}

		nlohmann::json manifest;
		manifest["version"] = kShaderCacheVersion;
		manifest["key"] = key;
		manifest["source"] = NormalizeCachePath(request.SourcePath);
		manifest["module"] = request.ModuleName;
		manifest["entry"] = request.EntryPointName;
		manifest["stage"] = StageToString(request.ShaderStage);
		manifest["profile"] = request.Profile;
		manifest["emitDebugInfo"] = request.EmitDebugInfo;
		manifest["optimize"] = request.Optimize;
		manifest["includeDirs"] = nlohmann::json::array();
		manifest["defines"] = nlohmann::json::object();

		for (const auto& includeDir : request.IncludeDirs)
			manifest["includeDirs"].push_back(NormalizeCachePath(includeDir));

		for (const auto& [name, value] : request.Defines)
			manifest["defines"][name] = value;

		const std::filesystem::path manifestPath = BuildManifestPath(key);
		std::error_code ec;
		std::filesystem::create_directories(manifestPath.parent_path(), ec);

		std::ofstream manifestFile(manifestPath);
		if (manifestFile.is_open())
			manifestFile << manifest.dump(4);

		KITA_CORE_INFO(
			"Shader cache stored: {} {}",
			request.SourcePath.string(),
			StageToString(request.ShaderStage));

		return true;

	}

	void ShaderCompileCache::ClearAll()
	{
		if (s_CacheRoot.empty())
			return;

		std::error_code ec;
		std::filesystem::remove_all(s_CacheRoot, ec);
		if (ec)
			KITA_CORE_WARN("Shader cache clear failed: {}", s_CacheRoot.string());
	}

	std::filesystem::path ShaderCompileCache::BuildSpirvPath(const std::string& key)
	{
		return s_CacheRoot / (key + ".spv");
	}

	std::filesystem::path ShaderCompileCache::BuildManifestPath(const std::string& key)
	{
		return s_CacheRoot / (key + ".json");
	}

	std::string ShaderCompileCache::BuildCacheKey(const ShaderCompiler::CompileRequest& request)
	{
		if (request.SourcePath.empty())
			return {};

		const std::string sourceBytes = ReadFileBytesAsString(request.SourcePath);
		if (sourceBytes.empty())
			return {};

		uint64_t hash = kFnvOffsetBasis;

		HashUInt32(hash, kShaderCacheVersion);
		HashString(hash, NormalizeCachePath(request.SourcePath));
		HashString(hash, sourceBytes);

		HashString(hash, request.ModuleName);
		HashString(hash, request.EntryPointName);
		HashString(hash, StageToString(request.ShaderStage));
		HashString(hash, request.Profile);

		// Include 搜索路径有顺序意义，不能排序。
		for (const auto& includeDir : request.IncludeDirs)
			HashString(hash, NormalizeCachePath(includeDir));

		std::vector<std::pair<std::string, std::string>> defines;
		defines.reserve(request.Defines.size());
		for (const auto& [name, value] : request.Defines)
			defines.emplace_back(name, value);

		std::sort(defines.begin(), defines.end(),
			[](const auto& left, const auto& right)
			{
				return left.first < right.first;
			});

		for (const auto& [name, value] : defines)
		{
			HashString(hash, name);
			HashString(hash, value);
		}

		HashBool(hash, request.EmitDebugInfo);
		HashBool(hash, request.Optimize);

		return ToHex(hash);
	}

}