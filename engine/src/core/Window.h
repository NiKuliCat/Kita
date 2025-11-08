#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "Core.h"
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
		~Window() {}

		static Ref<Window> Create(const WindowDescriptor& windowDescriptor) { return CreateRef<Window>(windowDescriptor); }
	public:
		void Init();
		void OnUpdate();
		void Destroy();

		GLFWwindow* GetNativeWindow() { return m_WindowHandle; }
		const uint32_t GetWidth() const { return m_Descriptor.Width; }
		const uint32_t GetHeight() const { return m_Descriptor.Height; }
	private:

	private:
		WindowDescriptor m_Descriptor;
		GLFWwindow* m_WindowHandle = nullptr;
	};
}


