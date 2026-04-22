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
		style.WindowBorderSize = 3.0f;
		style.TabBarBorderSize = 0.0f;
		ImFont* mainFont = io.Fonts->AddFontFromFileTTF("assets/fonts/Poppins/Poppins-Regular.ttf", 18.0f, nullptr );
		KITA_CORE_ASSERT(mainFont, "Failed to load ImGui font!");
		io.FontDefault = mainFont;

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		auto window = (GLFWwindow*)Application::Get().GetWindow()->GetNativeWindow();
		ImGui_ImplGlfw_InitForOpenGL(window, true);
		ImGui_ImplOpenGL3_Init("#version 410");

	}
	void ImGuiLayer::OnDestroy()
	{
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

		colors[ImGuiCol_WindowBg] = ImVec4{ 0.11f, 0.122f, 0.148f, 1.0f };

		//header
		colors[ImGuiCol_Header] = ImVec4{ 0.25f,0.255f,0.26f,1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.35f,0.355f,0.36f,1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f,0.155f,0.16f,1.0f };

		//button
		colors[ImGuiCol_Button] = ImVec4{ 0.25f,0.255f,0.26f,1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.35f,0.355f,0.36f,1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f,0.155f,0.16f,1.0f };

		//frame background
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.25f,0.255f,0.26f,1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.35f,0.355f,0.36f,1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f,0.155f,0.16f,1.0f };

		//tab
		colors[ImGuiCol_Tab] = ImVec4{ 0.11f,0.111f,0.115f,1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.11f,0.111f,0.115f,1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.11f,0.111f,0.115f,1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.11f,0.111f,0.115f,1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.11f,0.111f,0.115f,1.0f };

		//title
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.055f,0.056f,0.060f,1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.055f,0.056f,0.060f,1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.055f,0.056f,0.060f,1.0f };


		//docking
		colors[ImGuiCol_Border] = ImVec4{ 0.055f,0.056f,0.060f,1.0f };
	}

	void ImGuiLayer::Begin()
	{
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