#pragma once
#include "ShaderLabType.h"

namespace Kita {

	struct ShaderLabParseError
	{
		std::string Message;
		int Line = 0;
		int Column = 0;
	};

	struct ShaderLabParseResult
	{
		bool Success = false;
		ShaderLabAssetDesc Asset;
		ShaderLabParseError Error;
	};
	
	// ShaderLab 文本解析器。
	// 第二步目标：
	// 1. 支持新版结构化语法
	// 2. 给出明确的行列错误
	// 3. 产出完整 ShaderLabAssetDesc
	class ShaderLabParser
	{
	public:
		static ShaderLabParseResult ParseFile(const std::filesystem::path& path);
		static ShaderLabParseResult ParseText(std::string_view source, const std::filesystem::path& sourcePath = {});
	};

}