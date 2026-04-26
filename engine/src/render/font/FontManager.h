#pragma once

#include "core/Core.h"
#include "imgui.h"

struct GLFWwindow;

namespace Kita {


	#define ICON_FON_CANCEL u8"\ue800"
	#define ICON_FON_DOWN u8"\ue801"
	#define ICON_FON_RIGHT_OPEN   u8"\ue802"

	struct ImGuiFontStyle
	{
		std::string Name;
		std::string FilePath;
		float BasePixelSize = 18.0f;
		int OversampleH = 2;
		int OversampleV = 2;
		bool PixelSnapH = true;

		// new
		bool MergeIntoPrevious = false;
		std::vector<ImWchar> GlyphRanges = {};
	};

	class FontManager
	{
	public:
		static void Init();
		static void Shutdown();

		static void RegisterImGuiFont(const ImGuiFontStyle& style);
		static void BuildImGuiFonts(GLFWwindow* window);
		static bool RebuildImGuiFontsIfNeeded(GLFWwindow* window);

		static ImFont* GetImGuiFont(const std::string& name);
		static ImFont* GetDefaultImGuiFont();
		static float GetCurrentDpiScale() { return s_CurrentDpiScale; }

	private:
		static float QueryWindowDpiScale(GLFWwindow* window);
		static void ClearRuntimeData();

	private:
		static inline std::vector<ImGuiFontStyle> s_ImGuiFontStyles = {};
		static inline std::unordered_map<std::string, ImFont*> s_ImGuiFonts = {};
		static inline std::string s_DefaultFontName = {};
		static inline float s_CurrentDpiScale = 1.0f;
		static inline bool s_Initialized = false;
	};
}
