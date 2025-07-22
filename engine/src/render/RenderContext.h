#pragma once
#include "core/Base.h"
#include <glad/glad.h>
#include "GLFW/glfw3.h"
namespace Kita {

	class RenderContext
	{
	public:
		RenderContext(GLFWwindow* windowHandle);
		~RenderContext(){}

		static Ref<RenderContext> Create(GLFWwindow* windowHandle) { return CreateRef<RenderContext>(windowHandle); }
	public:
		void Init();
		void SwapBuffers();
	private:
		GLFWwindow* m_WindowHandle = nullptr;
	};
}