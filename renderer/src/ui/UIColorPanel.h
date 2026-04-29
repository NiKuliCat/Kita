#pragma once
#include <Engine.h>

#include <array>

namespace Kita {

	class UIColorPanel
	{
	public:
		void OnImGuiRender();

		void SetOpen(bool isOpen) { m_IsOpen = isOpen; }
		bool IsOpen() const { return m_IsOpen; }

	private:
		void EnsureCapturedColors();
		void CaptureCurrentColors();
		void ResetToCapturedColors();

		void DrawToolbar();
		void DrawPreview();
		void DrawColorEditor();
		void DrawColorRow(int colorIndex);

		void SaveCurrentColors();
		bool HasVisibleColorInGroup(const char* groupName) const;
		bool PassesFilter(const char* colorName, const char* groupName) const;

	private:
		std::vector<glm::vec4> m_CapturedColors;
		std::array<char, 128> m_SearchBuffer{};
		std::filesystem::path m_LastSavedPath;
		std::string m_StatusMessage;
		bool m_IsOpen = true;
		bool m_LastSaveSucceeded = false;
	};
}
