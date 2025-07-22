#include "Kitapch.h"

#include "RenderContext.h"

namespace Kita {
	RenderContext::RenderContext(GLFWwindow* windowHandle)
		:m_WindowHandle(windowHandle)
	{
		Init();
	}
	void RenderContext::Init()
	{
		Log::Message("init OpenGL context");
		glfwMakeContextCurrent(m_WindowHandle);

		int status = gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
		auto message	= "OpenGL version : " + (std::string)reinterpret_cast<const char*>(glGetString(GL_VERSION));
		Log::Message(message);
		message			= "GPU :  " + (std::string)reinterpret_cast<const char*>(glGetString(GL_RENDERER));
		Log::Message(message);
	}
	void RenderContext::SwapBuffers()
	{
		glfwSwapBuffers(m_WindowHandle);
	}
}