#include "renderer_pch.h"
#include "FileDialogs.h"
#include <windows.h>
#include <commdlg.h>
namespace Kita{
	std::filesystem::path FileDialogs::OpenFile(const wchar_t* filter)
	{
		wchar_t fileBuffer[4096] = { 0 };

		OPENFILENAMEW ofn{};
		ofn.lStructSize = sizeof(OPENFILENAMEW);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFile = fileBuffer;
		ofn.nMaxFile = sizeof(fileBuffer) / sizeof(wchar_t);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_NOCHANGEDIR;

		if (GetOpenFileNameW(&ofn) == TRUE)
		{
			return std::filesystem::path(fileBuffer);
		}

		return {};
	}
	std::filesystem::path FileDialogs::SaveFile(const wchar_t* filter, const wchar_t* defaultExtension)
	{
		wchar_t fileBuffer[4096] = { 0 };

		OPENFILENAMEW ofn{};
		ofn.lStructSize = sizeof(OPENFILENAMEW);
		ofn.hwndOwner = nullptr;
		ofn.lpstrFile = fileBuffer;
		ofn.nMaxFile = sizeof(fileBuffer) / sizeof(wchar_t);
		ofn.lpstrFilter = filter;
		ofn.nFilterIndex = 1;
		ofn.lpstrDefExt = defaultExtension;
		ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

		if (GetSaveFileNameW(&ofn) == TRUE)
		{
			return std::filesystem::path(fileBuffer);
		}

		return {};
	}
}