#pragma once

#include "core/Core.h"
#include "imgui.h"

struct GLFWwindow;

namespace Kita {


	#define ICON_FON_CANCEL "\ue800"
	#define ICON_FON_DOWN "\ue801"
	#define ICON_FON_RIGHT_OPEN   "\ue802"
	#define ICON_FON_VIDEOCAM "\ue803"
	#define ICON_FON_PICTURE "\ue804"
	#define ICON_FON_CAMERA "\ue805"
	#define ICON_FON_TH_LARGE "\ue806"
	#define ICON_FON_TH "\ue807"
	#define ICON_FON_TH_LIST "\ue808"
	#define ICON_FON_LOCK "\ue809"
	#define ICON_FON_EYE "\ue80a"
	#define ICON_FON_EYE_OFF "\ue80b"
	#define ICON_FON_TAG "\ue80c"
	#define ICON_FON_TAGS "\ue80d"
	#define ICON_FON_CCW "\ue80e"
	#define ICON_FON_PLAY "\ue80f"
	#define ICON_FON_STOP "\ue810"
	#define ICON_FON_PAUSE "\ue811"
	#define ICON_FON_LIST "\ue812"
	#define ICON_FON_CHECK "\ue813"
	#define ICON_FON_MENU_1 "\ue814"
	#define ICON_FON_SEARCH "\ue815"
	#define ICON_FON_LAYOUT "\ue816"
	#define ICON_FON_BACK "\ue817"
	#define ICON_FON_LOCK_1 "\ue818"
	#define ICON_FON_LOCK_OPEN "\ue819"
	#define ICON_FON_LIGHT_UP "\ue81a"
	#define ICON_FON_CCW_1 "\ue81b"
	#define ICON_FON_FOLDER "\ue81c"
	#define ICON_FON_FOLDER_OPEN "\ue81d"
	#define ICON_FON_MENU "\uf0c9"
	#define ICON_FON_DOC_TEXT "\uf0f6"
	#define ICON_FON_ANGLE_RIGHT "\uf105"
	#define ICON_FON_ANGLE_DOWN "\uf107"
	#define ICON_FON_FOLDER_EMPTY "\uf114"
	#define ICON_FON_FOLDER_OPEN_EMPTY "\uf115"
	#define ICON_FON_LOCK_OPEN_ALT "\uf13e"
	#define ICON_FON_SUN "\uf185"
	#define ICON_FON_CUBE "\uf1b2"

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
