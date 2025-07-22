#include "Kitapch.h"
#include "Window.h"

namespace Kita {

	Window::Window(const WindowDescriptor& windowDescriptor)
		:m_Descriptor(windowDescriptor)
	{
		Init();
	}

	void Window::Init()
	{
		if (!glfwInit())
		{
			std::cerr << "Could not initalize GLFW!\n";
			return;
		}
		//glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		//glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
		m_WindowHandle = glfwCreateWindow(m_Descriptor.Width, m_Descriptor.Height, m_Descriptor.Title.c_str(), nullptr, nullptr);

		if (!m_WindowHandle)
		{
			std::cerr << "Could not create GLFW window!\n";
			return;
		}
	}

	void Window::OnUpdate()
	{
		glfwPollEvents();
	}

	void Window::Destroy()
	{
		glfwDestroyWindow(m_WindowHandle);
		glfwTerminate();
	}


}