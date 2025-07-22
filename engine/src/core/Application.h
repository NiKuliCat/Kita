#pragma once
#include "Base.h"
#include "Window.h"
#include "render/RenderContext.h"
namespace Kita{

	struct ApplicationDescriptor
	{
		std::string Name;
		uint32_t Width;
		uint32_t Height;
	};

	class Application
	{
	public:
		Application(const ApplicationDescriptor& appDescriptor);
		virtual ~Application() {}


	public:
		void Run();
		void InitWindow();
		void InitRenderContext();

		void MainLoop();
		void Shutdown();
		void SetActive(bool active) { m_Active = active; }
	private:
		ApplicationDescriptor m_Descriptor{};
		bool m_Active;

		Ref<Window> m_Window = nullptr;
		Ref<RenderContext> m_RenderContext = nullptr;
	};

	Application* CreateApplication(int argc, char** argv);
}