#pragma once

#include "core/Core.h"
#include "imgui.h"

struct GLFWwindow;

namespace Kita {


	#define ICON_FON_CANCEL u8"\ue800"
	#define ICON_FON_DOWN u8"\ue801"
	#define ICON_FON_RIGHT_OPEN   u8"\ue802"
	#define ICON_FON_VIDEOCAM u8"\ue803"
	#define ICON_FON_PICTURE u8"\ue804"
	#define ICON_FON_CAMERA u8"\ue805"
	#define ICON_FON_TH_LARGE u8"\ue806"
	#define ICON_FON_TH u8"\ue807"
	#define ICON_FON_TH_LIST u8"\ue808"
	#define ICON_FON_LOCK u8"\ue809"
	#define ICON_FON_EYE u8"\ue80a"
	#define ICON_FON_EYE_OFF u8"\ue80b"
	#define ICON_FON_TAG u8"\ue80c"
	#define ICON_FON_TAGS u8"\ue80d"
	#define ICON_FON_CCW u8"\ue80e"
	#define ICON_FON_PLAY u8"\ue80f"
	#define ICON_FON_STOP u8"\ue810"
	#define ICON_FON_PAUSE u8"\ue811"
	#define ICON_FON_LIST u8"\ue812"
	#define ICON_FON_CHECK u8"\ue813"
	#define ICON_FON_MENU_1 u8"\ue814"
	#define ICON_FON_SEARCH u8"\ue815"
	#define ICON_FON_LAYOUT u8"\ue816"
	#define ICON_FON_BACK u8"\ue817"
	#define ICON_FON_LOCK_1 u8"\ue818"
	#define ICON_FON_LOCK_OPEN u8"\ue819"
	#define ICON_FON_LIGHT_UP u8"\ue81a"
	#define ICON_FON_CCW_1 u8"\ue81b"
	#define ICON_FON_FOLDER u8"\ue81c"
	#define ICON_FON_FOLDER_OPEN u8"\ue81d"
	#define ICON_FON_MENU u8"\uf0c9"
	#define ICON_FON_DOC_TEXT u8"\uf0f6"
	#define ICON_FON_ANGLE_RIGHT u8"\uf105"
	#define ICON_FON_ANGLE_DOWN u8"\uf107"
	#define ICON_FON_FOLDER_EMPTY u8"\uf114"
	#define ICON_FON_FOLDER_OPEN_EMPTY u8"\uf115"
	#define ICON_FON_LOCK_OPEN_ALT u8"\uf13e"
	#define ICON_FON_SUN u8"\uf185"
	#define ICON_FON_CUBE u8"\uf1b2"

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
