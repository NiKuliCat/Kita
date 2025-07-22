#include "Kitapch.h"

#include "Application.h"
#include <GLFW/glfw3.h>
namespace Kita{
	Application::Application(const ApplicationDescriptor& appDescriptor)
		:m_Descriptor(appDescriptor)
	{
		std::string message = "launch current active app :  " + m_Descriptor.Name;
		Log::Message(message);

		m_Active = true;
	}
	void Application::Run()
	{
		InitWindow();

		InitRenderContext();

		MainLoop();

		Shutdown();
	}
	void Application::InitWindow()
	{
		WindowDescriptor descriptor{};
		descriptor.Title = m_Descriptor.Name;
		descriptor.Width = m_Descriptor.Width;
		descriptor.Height = m_Descriptor.Height;

		m_Window = Window::Create(descriptor);

		glfwSetWindowUserPointer(m_Window->GetNativeWindow(), this);
		glfwSetWindowCloseCallback(m_Window->GetNativeWindow(), [](GLFWwindow* window) {
			auto app = reinterpret_cast<Application*>(glfwGetWindowUserPointer(window));
			app->SetActive(false);
			});



	}
	void Application::InitRenderContext()
	{
		m_RenderContext = RenderContext::Create(m_Window->GetNativeWindow());
	}
	void Application::MainLoop()
	{
		while (m_Active)
		{
			m_Window->OnUpdate();
			m_RenderContext->SwapBuffers();
		}
	}
	void Application::Shutdown()
	{
		Log::Message("app shutdown");
		m_Window->Destroy();
	}
}