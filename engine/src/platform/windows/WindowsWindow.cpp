#include "kita_pch.h"
#include "WindowsWindow.h"
#include "core/Log.h"

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
		m_WindowData.GraphicsAPI = descriptor.GraphicsAPI;


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

		m_Context = GraphicsContext::Create(m_Window, m_WindowData.GraphicsAPI);
		m_Context->Init();

		glfwSetWindowUserPointer(m_Window, this);
		SetVSync(false);

#pragma region ----------------------------------------------------glfw callback

		glfwSetWindowSizeCallback(m_Window, [](GLFWwindow* window, int width, int height)
			{
				WindowsWindow* self = static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

				self->m_WindowData.Width = width;
				self->m_WindowData.Height = height;

				if (self->m_Context)
				{
					self->m_Context->OnResize((uint32_t)width, (uint32_t)height);
				}

				WindowResizeEvent event(width, height);
				self->m_WindowData.EventCallback(event);
			});

		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				WindowsWindow* self = static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));
				WindowCloseEvent event;
				self->m_WindowData.EventCallback(event);
			});

		glfwSetCharCallback(m_Window, [](GLFWwindow* window, unsigned int codepoint)
			{
				WindowsWindow* self = static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));
				KeyTypedEvent event(codepoint);
				self->m_WindowData.EventCallback(event);
			});


		glfwSetKeyCallback(m_Window, [](GLFWwindow* window, int key, int scancode, int  action, int mods)
			{
				WindowsWindow* self = static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

				switch (action)
				{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					self->m_WindowData.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					self->m_WindowData.EventCallback(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1);
					self->m_WindowData.EventCallback(event);
					break;
				}
				}
			});

		glfwSetMouseButtonCallback(m_Window, [](GLFWwindow* window, int button, int action, int mods)
			{
				WindowsWindow* self = static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));

				switch (action)
				{
				case GLFW_PRESS:
				{
					MouseButtonPressedEvent event(button);
					self->m_WindowData.EventCallback(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseButtonReleaseEvent event(button);
					self->m_WindowData.EventCallback(event);
					break;
				}
				}

			});



		glfwSetScrollCallback(m_Window, [](GLFWwindow* window, double xoffset, double yoffset)
			{
				WindowsWindow* self = static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));
				MouseScrolledEvent event((float)xoffset, (float)yoffset);
				self->m_WindowData.EventCallback(event);
			});

		glfwSetCursorPosCallback(m_Window, [](GLFWwindow* window, double xpos, double ypos)
			{
				WindowsWindow* self = static_cast<WindowsWindow*>(glfwGetWindowUserPointer(window));
				MouseMovedEvent event((float)xpos, (float)ypos);
				self->m_WindowData.EventCallback(event);
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