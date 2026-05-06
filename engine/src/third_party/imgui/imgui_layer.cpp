#include "kita_pch.h"
#include "imgui_layer.h"

#include "imgui.h"
#include "ImGuizmo.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include "core/Application.h"
#include "core/Log.h"
#include "render/font/FontManager.h"
namespace Kita {
	ImGuiLayer::ImGuiLayer()
		:Layer("ImGui Layer")
	{
	}
	ImGuiLayer::~ImGuiLayer()
	{
	}
	void ImGuiLayer::OnCreate()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;


		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

		ImGui::StyleColorsDark();

		SetDarkThemeSpace();
		ImGuiStyle& style = ImGui::GetStyle();
		style.FramePadding.y = 2.0f; //title栏高度
		style.WindowBorderSize = 0.0f;

		style.TabRounding = 0.0f;
		style.TabBorderSize = 0.0f;
		style.TabBarBorderSize = 0.0f;


		style.ChildRounding = 6.0f;
		style.FrameRounding = 4.0f;
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		auto window = (GLFWwindow*)Application::Get().GetWindow()->GetNativeWindow();
		FontManager::Init();
		FontManager::RegisterImGuiFont({ "Default", "packages/editor/fonts/Poppins/Poppins-Regular.ttf", 20.0f, 2, 2, true });
		FontManager::RegisterImGuiFont({ "FontelloIcons", "packages/editor/icons/fontello-fe918da2/fontello-fe918da2/fontello.ttf",32.0f,3,3,true,true,{0xe800, 0xe81d, 0xf0c9, 0xf0c9, 0xf0f6, 0xf0f6, 0xf105, 0xf107, 0xf114, 0xf115, 0xf13e, 0xf13e, 0xf185, 0xf185, 0xf1b2, 0xf1b2, 0}});

		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 410");
		FontManager::BuildImGuiFonts(window);

	}
	void ImGuiLayer::OnDestroy()
	{
		FontManager::Shutdown();
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}
	void ImGuiLayer::OnUpdate(float daltaTime)
	{

	}
	void ImGuiLayer::OnImGuiRender()
	{
		//ImGui::ShowDemoWindow();
	}

	void ImGuiLayer::SetDarkThemeSpace()
	{
		auto& colors = ImGui::GetStyle().Colors;

		//header  最顶部颜色（file window ）
		colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.01, 0.01, 0.01, 1.0 };

		colors[ImGuiCol_WindowBg] = ImVec4{ 0.177,0.177, 0.177, 1.0 };

		//header
		colors[ImGuiCol_Header] = ImVec4{ 0.246f,0.467f,0.632f,0.645f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.420f,0.420f,0.420f,0.645f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.246f,0.467f,0.632f,0.645f };

		//button
		colors[ImGuiCol_Button] = ImVec4{ 0.25f,0.255f,0.26f,0.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.35f,0.355f,0.36f,1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f,0.155f,0.16f,1.0f };

		//frame background
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.20, 0.20, 0.20, 1.0 };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.260, 0.20, 0.20, 1.0 };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.20, 0.20, 0.20, 1.0 };

		//tab
		colors[ImGuiCol_Tab] = ImVec4{ 0.157, 0.157, 0.157, 1.0 };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.157, 0.157, 0.157, 1.0 };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.157, 0.157, 0.157, 1.0 };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.157, 0.157, 0.157, 1.0 };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{0.157, 0.157, 0.157, 1.0};
		colors[ImGuiCol_TabSelectedOverline] = ImVec4{ 0.000,1.000,0.416,1.0 };
		colors[ImGuiCol_TabDimmedSelected] = ImVec4{ 0.177,0.177, 0.177, 1.0 };


		//title  子窗口顶部栏配色
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.102, 0.102, 0.102, 1.0 };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.102, 0.102, 0.102, 1.0 };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.102, 0.102, 0.102, 1.0 };



		//docking
		colors[ImGuiCol_Border] = ImVec4{ 0.011f,0.011f,0.011f,1.0f };
	}

	void ImGuiLayer::Begin()
	{
		auto window = (GLFWwindow*)Application::Get().GetWindow()->GetNativeWindow();
		FontManager::RebuildImGuiFontsIfNeeded(window);
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
		ImGuizmo::BeginFrame();

	}

	void ImGuiLayer::End()
	{
		ImGuiIO& io = ImGui::GetIO();
		Application& app = Application::Get();
		io.DisplaySize = ImVec2((float)app.GetWindow()->GetWidth(), (float)app.GetWindow()->GetHeight());


		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
	}
}
