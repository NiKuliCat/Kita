#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "Base.h"

namespace Kita {

	struct WindowDescriptor
	{
		std::string Title;
		uint32_t Width;
		uint32_t Height;
	};

	class Window
	{
	public:
		Window(const WindowDescriptor& windowDescriptor);
		~Window(){}

		static Ref<Window> Create(const WindowDescriptor& windowDescriptor) { return CreateRef<Window>(windowDescriptor); }
	public:
		void Init();
		void OnUpdate();
		void Destroy();

		GLFWwindow* GetNativeWindow() { return m_WindowHandle; }
	private:

	private:
		WindowDescriptor m_Descriptor;
		GLFWwindow* m_WindowHandle = nullptr;
	};
}
