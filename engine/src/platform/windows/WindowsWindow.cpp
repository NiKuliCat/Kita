#include "kita_pch.h"
#include "WindowsWindow.h"
#include "core/Log.h"
#include "platform/opengl/OpenGLContext.h"

#include "event/KeyBoardEvent.h"
#include "event/MouseEvent.h"
#include "event/ApplicationEvent.h"
namespace Kita {

	static bool s_GLFWInitialized = false;

	static void GLFWErrorCallbackFunc(int error, const char* discription)
	{
		KITA_CORE_ERROR("GLFW Error {0} : {1}", error, discription);
	}

	Ref<Window> Window::Create(const WindowDescriptor& windowDescriptor)
	{
		return CreateRef<WindowsWindow>(windowDescriptor);
	}

	WindowsWindow::WindowsWindow(const WindowDescriptor& descriptor)
	{
		Init(descriptor);

	}

	void WindowsWindow::Init(const WindowDescriptor& descriptor)
	{
		m_WindowData.Title = descriptor.Title;
		m_WindowData.Width = descriptor.Width;
		m_WindowData.Height = descriptor.Height;

		KITA_CORE_INFO("Create Window {0} ({1},{2})", descriptor.Title, descriptor.Width, descriptor.Height);

		if (!s_GLFWInitialized )
		{
			int success = glfwInit();

			KITA_CORE_ASSERT(success, "Could not init GLFW !");
			glfwSetErrorCallback(GLFWErrorCallbackFunc);

			s_GLFWInitialized = true;
		}

		m_Window = glfwCreateWindow((int)m_WindowData.Width, (int)m_WindowData.Height, m_WindowData.Title.c_str(), nullptr, nullptr);

		if (!m_Window)
		{
			const char* error;
			glfwGetError(&error);
			KITA_CORE_ERROR("Window creation failed: {0}", error);
			exit(1);
		}

		m_Context = new OpenGLContext(m_Window);
		m_Context->Init();

		glfwSetWindowUserPointer(m_Window, &m_WindowData);
		SetVSync(false);

#pragma region ----------------------------------------------------glfw callback

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				data.Width = width;
				data.Height = height;
				WindowResizeEvent event(width, height);
				KITA_CORE_INFO(event.ToString());
				data.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				WindowCloseEvent event;
				data.EventCallback(event);
			});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int codepoint)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				KeyTypedEvent event(codepoint);
				data.EventCallback(event);
			});


		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int  action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					data.EventCallback(event);
					break;
				}
				}
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

				switch (action)
				{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					data.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleaseEvent event(button);
					data.EventCallback(event);
					break;
				}
				}

			});



		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xoffset, double yoffset)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				MouseScrolledEvent event((float)xoffset, (float)yoffset);
				data.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xpos, double ypos)
			{
				WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
				MouseMovedEvent event((float)xpos, (float)ypos);
				data.EventCallback(event);
			});
#pragma endregion
	}

	WindowsWindow::~WindowsWindow()
	{
		OnDestroy();
	}

	void WindowsWindow::SetVSync(bool enabled)
	{
		if (enabled)
		{
			glfwSwapInterval(1);
		}
		else
		{
			glfwSwapInterval(0);
		}
		m_WindowData.VSync = enabled;
	}

	void WindowsWindow::OnUpdate()
	{
		glfwPollEvents();
		m_Context->SwapBuffers();
	}

	void WindowsWindow::OnDestroy()
	{
		glfwDestroyWindow(m_Window);
	}



}