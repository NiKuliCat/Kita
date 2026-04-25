#include "kita_pch.h"
#include "FontManager.h"

#include <GLFW/glfw3.h>
#include <backends/imgui_impl_opengl3.h>
#include "core/Log.h"

namespace Kita {

	void FontManager::Init()
	{
		if (s_Initialized)
			return;

		s_ImGuiFontStyles.clear();
		s_ImGuiFonts.clear();
		s_DefaultFontName.clear();
		s_CurrentDpiScale = 1.0f;
		s_Initialized = true;
	}

	void FontManager::Shutdown()
	{
		ClearRuntimeData();
		s_ImGuiFontStyles.clear();
		s_Initialized = false;
	}

	void FontManager::RegisterImGuiFont(const ImGuiFontStyle& style)
	{
		KITA_CORE_ASSERT(s_Initialized, "FontManager::Init must be called before RegisterImGuiFont.");
		KITA_CORE_ASSERT(!style.Name.empty(), "ImGui font name must not be empty.");
		KITA_CORE_ASSERT(!style.FilePath.empty(), "ImGui font file path must not be empty.");

		const auto found = std::find_if(
			s_ImGuiFontStyles.begin(),
			s_ImGuiFontStyles.end(),
			[&style](const ImGuiFontStyle& current) { return current.Name == style.Name; });

		if (found != s_ImGuiFontStyles.end())
		{
			*found = style;
		}
		else
		{
			s_ImGuiFontStyles.push_back(style);
		}

		if (s_DefaultFontName.empty())
		{
			s_DefaultFontName = style.Name;
		}
	}

	void FontManager::BuildImGuiFonts(GLFWwindow* window)
	{
		KITA_CORE_ASSERT(s_Initialized, "FontManager::Init must be called before BuildImGuiFonts.");
		KITA_CORE_ASSERT(window, "BuildImGuiFonts requires a valid GLFWwindow.");

		ImGuiIO& io = ImGui::GetIO();
		ImGui_ImplOpenGL3_DestroyFontsTexture();
		io.Fonts->Clear();
		ClearRuntimeData();

		s_CurrentDpiScale = QueryWindowDpiScale(window);

		for (const auto& style : s_ImGuiFontStyles)
		{
			ImFontConfig config{};
			config.OversampleH = style.OversampleH;
			config.OversampleV = style.OversampleV;
			config.PixelSnapH = style.PixelSnapH;

			ImFont* font = io.Fonts->AddFontFromFileTTF(
				style.FilePath.c_str(),
				style.BasePixelSize * s_CurrentDpiScale,
				&config);

			KITA_CORE_ASSERT(font, "Failed to load ImGui font.");
			s_ImGuiFonts[style.Name] = font;
		}

		auto defaultFont = GetDefaultImGuiFont();
		KITA_CORE_ASSERT(defaultFont, "No default ImGui font was built.");
		io.FontDefault = defaultFont;

		ImGui_ImplOpenGL3_CreateFontsTexture();
	}

	bool FontManager::RebuildImGuiFontsIfNeeded(GLFWwindow* window)
	{
		KITA_CORE_ASSERT(window, "RebuildImGuiFontsIfNeeded requires a valid GLFWwindow.");

		const float newScale = QueryWindowDpiScale(window);
		if (std::abs(newScale - s_CurrentDpiScale) < 0.01f)
			return false;

		BuildImGuiFonts(window);
		return true;
	}

	ImFont* FontManager::GetImGuiFont(const std::string& name)
	{
		const auto found = s_ImGuiFonts.find(name);
		if (found == s_ImGuiFonts.end())
			return nullptr;

		return found->second;
	}

	ImFont* FontManager::GetDefaultImGuiFont()
	{
		if (s_DefaultFontName.empty())
			return nullptr;

		return GetImGuiFont(s_DefaultFontName);
	}

	float FontManager::QueryWindowDpiScale(GLFWwindow* window)
	{
		float xscale = 1.0f;
		float yscale = 1.0f;
		glfwGetWindowContentScale(window, &xscale, &yscale);
		return std::max(1.0f, std::max(xscale, yscale));
	}

	void FontManager::ClearRuntimeData()
	{
		s_ImGuiFonts.clear();
	}
}
