#pragma once

#include "render/GraphicsContext.h"


struct GLFWwindow;


namespace Kita {

	class OpenGLContext : public GraphicsContext
	{
	public:
		OpenGLContext(GLFWwindow* windowHandle);

		virtual void Init() override;
		virtual void SwapBuffers() override;
		virtual void OnResize(uint32_t width, uint32_t height)  override;

		virtual RendererAPI::API GetAPI() const override { return RendererAPI::API::OpenGL; }

	private:
		GLFWwindow* m_WindowHandle;
	};
}