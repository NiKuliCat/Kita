#pragma once
#include "Core.h"
#include "event/Event.h"	
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
		using EventCallbackFn = std::function<void(Event&)>; // 函数指针 接收 返回值为空 参数为event
		virtual ~Window() = default;

	public:
		virtual void OnUpdate() = 0;
		virtual void OnDestroy() = 0;

		virtual void* GetNativeWindow()  const = 0;
		virtual uint32_t GetWidth() const = 0;
		virtual uint32_t GetHeight() const = 0;

		virtual void SetVSync(bool enabled) = 0;
		virtual bool isVSync() const = 0;

		virtual void SetEventCallback(const EventCallbackFn& callback) = 0;

		static Ref<Window> Create(const WindowDescriptor& windowDescriptor);
	};
}


