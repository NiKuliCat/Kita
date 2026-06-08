#pragma once

#include "ShaderLabType.h"

namespace Kita {

	struct MaterialParseError
	{
		std::string Message;
		int Line = 0;
		int Column = 0;
	};

	struct MaterialParseResult
	{
		bool Success = false;
		MaterialDefinitionDesc Asset;
		MaterialParseError Error;
	};

	class MaterialDefinitionParser
	{
	public:
		static MaterialParseResult ParseFile(const std::filesystem::path& path);
		static MaterialParseResult ParseText(std::string_view source, const std::filesystem::path& sourcePath = {});
	};

	using ShaderLabParseError = MaterialParseError;
	using ShaderLabParseResult = MaterialParseResult;
	using ShaderLabParser = MaterialDefinitionParser;

}
