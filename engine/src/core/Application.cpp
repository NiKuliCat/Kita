#include "EnginePch.h"
#include "Application.h"

#include "Base.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
namespace Kita {

	Application* Application::s_Instance = nullptr;
	Application::Application(const ApplicationDescriptor& app_descriptor)
		:m_Descriptor(app_descriptor)
	{
		s_Instance = this;
		std::string message = "launch current active app :  " + m_Descriptor.name;

		Log::Message(message);

		m_Active = true;
	}

	void Application::Run()
	{
		Log::Message("engine core running");
		InitWindow();
		InitImGuiLayer();
		MainLoop();

		ShutDown();
	}

	void Application::InitWindow()
	{
		WindowDescriptor descriptor{};
		descriptor.Title = m_Descriptor.name;
		descriptor.Width = m_Descriptor.width;
		descriptor.Height = m_Descriptor.height;

		m_Window = Window::Create(descriptor);

		glfwSetWindowUserPointer(m_Window->GetNativeWindow(), this);
		glfwSetWindowCloseCallback(m_Window->GetNativeWindow(), [](GLFWwindow* window) {
			auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
			app->SetActive(false);
			});
	}

	void Application::InitImGuiLayer()
	{
		m_ImGuiLayer = new ImGuiLayer();
		PushOverlay(m_ImGuiLayer);
		Log::Message("init imguiLayer");
	}

	void Application::MainLoop()
	{
		while (m_Active)
		{
			ReSize();
			m_ImGuiLayer->Begin();
			for (Layer* layer : m_LayerStack)
			{
				layer->OnImGuiRender();
			}
			m_ImGuiLayer->End();
			
			m_Window->OnUpdate();
			
		}

	}

	void Application::ShutDown()
	{
		m_Window->Destroy();
	}



	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnCreate();
	}
	void Application::PushOverlay(Layer* overlay)
	{
		m_LayerStack.PushOverlay(overlay);
		overlay->OnCreate();
	}
	void Application::ReSize()
	{
		int display_w, display_h;
		glfwGetFramebufferSize(m_Window->GetNativeWindow(), &display_w, &display_h);
		glViewport(0, 0, display_w, display_h);
		glClearColor(0.3, 0.02, 0.04, 1.0);
		glClear(GL_COLOR_BUFFER_BIT);
	}
}