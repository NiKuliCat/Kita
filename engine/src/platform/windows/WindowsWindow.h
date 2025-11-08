#pragma once
#include "core/Window.h"
#include "core/Core.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "render/GraphicsContext.h"
namespace  Kita {


	class WindowsWindow :public Window
	{
	public:
		WindowsWindow(const WindowDescriptor& descriptor);

		virtual ~WindowsWindow();

	
		virtual void OnUpdate() override;
		virtual void OnDestroy() override;
		inline uint32_t GetWidth() const  override { return m_WindowData.Width; }
		inline uint32_t GetHeight() const  override { return m_WindowData.Height; }

		inline void SetEventCallback(const EventCallbackFn& callback) override { m_WindowData.EventCallback = callback; }

		void SetVSync(bool enabled) override;
		bool isVSync() const override { return m_WindowData.VSync; }

		inline virtual void* GetNativeWindow() const  override { return m_Window; };

	private:
		void Init(const WindowDescriptor& descriptor);

	private:

		struct WindowData
		{
			std::string Title;
			uint32_t Width, Height;

			bool VSync;
			EventCallbackFn EventCallback;
		};
		GLFWwindow* m_Window { nullptr };
		GraphicsContext* m_Context { nullptr };
		WindowData m_WindowData;
	};
}