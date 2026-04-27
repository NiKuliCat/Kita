#pragma once
#include "Engine.h"
namespace Kita {

	class FileDialogs
	{
	public:
		static std::filesystem::path OpenFile(const wchar_t* filter);
		static std::filesystem::path SaveFile(const wchar_t* filter, const wchar_t* defaultExtension = L"");
	};
}