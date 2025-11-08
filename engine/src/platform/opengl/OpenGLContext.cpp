#include "kita_pch.h"
#include "OpenGLContext.h"
#include "core/Core.h"
#include "core/Log.h"
#include <glad/glad.h>
#include "GLFW/glfw3.h"
namespace Kita {



	OpenGLContext::OpenGLContext(GLFWwindow* windowHandle)
		:m_WindowHandle(windowHandle)
	{
		KITA_CORE_ASSERT(windowHandle, "window handle is null !");


	}


	void OpenGLContext::Init()
	{
		glfwMakeContextCurrent(m_WindowHandle);
		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		KITA_CORE_ASSERT(status, "Failed to init Glad !");
		KITA_CORE_INFO("OpenGL-Version: {0}", reinterpret_cast<const char*>(glGetString(GL_VERSION)));
		KITA_CORE_INFO("OpenGLRenderer: {0}", reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
	}

	void OpenGLContext::SwapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
	}

}