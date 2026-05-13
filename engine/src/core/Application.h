#pragma once
#include "Core.h"
#include "Window.h"
#include "Layer.h"
#include "third_party/imgui/imgui_layer.h"

#include "event/ApplicationEvent.h"

#include "render/VulkanContext.h"
#include "system/TimeSystem.h"
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
		void InitVulkanContext();
		void InitImGuiLayer();
		void InitRenderer();
		void MainLoop();
		void ShutDown();
		void InitializePendingLayers();

		void OnEvent(Event& event);

		void SetActive(bool active) { m_Active = active; }


		inline static Application& Get() { return *s_Instance; }
		inline Ref<Window>& GetWindow() { return m_Window; }


		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);

		inline VulkanContext& GetVulkanContext() { return *m_VulkanContext; }
		inline const VulkanContext& GetVulkanContext() const { return *m_VulkanContext; }

		inline TimeSystem& GetTimeSystem() { return m_TimeSystem; }
		inline const TimeSystem& GetTimeSystem() const { return m_TimeSystem; }
	private:
		bool OnWindowClosed(WindowCloseEvent& event);
		bool OnWindowResize(WindowResizeEvent& event);




	private:
		ApplicationDescriptor m_Descriptor{};

		Ref<Window> m_Window = nullptr;
		Unique<VulkanContext> m_VulkanContext = nullptr;
		TimeSystem m_TimeSystem;
		LayerStack m_LayerStack;
		ImGuiLayer* m_ImGuiLayer = nullptr;
		bool m_Active = false;
		bool m_Minimized = false;
		bool m_LayersInitialized = false;
		static Application* s_Instance;
	};


	Application* CreateApplication(int argc, char** argv);

}
