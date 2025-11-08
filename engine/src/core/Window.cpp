#include "EnginePch.h"
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
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		m_WindowHandle = glfwCreateWindow(m_Descriptor.Width, m_Descriptor.Height, m_Descriptor.Title.c_str(), nullptr, nullptr);

		glfwMakeContextCurrent(m_WindowHandle);
		glfwSwapInterval(1); // Enable vsync
		if (!m_WindowHandle)
		{
			std::cerr << "Could not create GLFW window!\n";
			return;
		}
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
			// ´¦Àí´íÎó
			glfwTerminate();
			return;
		}
	}

	void Window::OnUpdate()
	{
		glfwPollEvents();
		glfwSwapBuffers(m_WindowHandle);
	}

	void Window::Destroy()
	{
		glfwDestroyWindow(m_WindowHandle);
		glfwTerminate();
	}

}