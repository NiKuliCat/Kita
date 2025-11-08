#pragma once
#include "Window.h"
#include "Layer.h"
#include "third_party/imgui/imgui_layer.h"
namespace Kita {

	struct ApplicationDescriptor
	{
		std::string name;
		uint32_t width;
		uint32_t height;
	};

	class Application
	{
	public:
		Application(const ApplicationDescriptor& app_descriptor);
		virtual ~Application() {};


	public:
		void Run();
		void InitWindow();
		void InitImGuiLayer();
		void MainLoop();
		void ShutDown();

		void SetActive(bool active) { m_Active = active; }


		inline static Application& Get() { return *s_Instance; }
		inline Ref<Window>& GetWindow() { return m_Window; }


		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);


	private:
		void ReSize();

	private:
		ApplicationDescriptor m_Descriptor{};

		Ref<Window> m_Window = nullptr;
		
		LayerStack m_LayerStack;
		ImGuiLayer* m_ImGuiLayer = nullptr;
		bool m_Active;
		static Application* s_Instance;
	};


	Application* CreateApplication(int argc, char** argv);

}