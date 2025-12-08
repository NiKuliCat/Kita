#include "kita_pch.h"
#include "Application.h"

#include "Log.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include "Input.h"
#include "event/KeyCode.h"

#include "render/Buffer.h"
#include "render/scene/OrthographicCamera.h"
#include "render/RenderCommand.h"
#include "component/Object.h"
#include "render/Renderer.h"
#include "render/Texture.h"
#include "render/ShaderLibrary.h"
#include  "render/FrameBuffer.h"
namespace Kita {

	Application* Application::s_Instance = nullptr;
	Application::Application(const ApplicationDescriptor& app_descriptor)
		:m_Descriptor(app_descriptor)
	{
		s_Instance = this;
		KITA_CORE_TRACE("launch current active app: " + m_Descriptor.name);

		m_Active = true;
		InitWindow();
		InitImGuiLayer();
	}

	void Application::Run()
	{
	
	

		MainLoop();
		ShutDown();
	}

	void Application::InitWindow()
	{
		WindowDescriptor descriptor{};
		descriptor.Title = m_Descriptor.name;
		descriptor.Width = m_Descriptor.width;
		descriptor.Height = m_Descriptor.height;

		m_Window = Window::Create(descriptor);
		m_Window->SetEventCallback(BIND_EVENT_FUNC(Application::OnEvent));
	}

	void Application::InitImGuiLayer()
	{
		m_ImGuiLayer = new ImGuiLayer();
		KITA_CORE_TRACE("init imgui layer");
		PushOverlay(m_ImGuiLayer);
	}

	void Application::MainLoop()
	{

		while (m_Active)
		{
			

			if (!m_Minimized)
			{
				for (Layer* layer : m_LayerStack)
				{
					layer->OnUpdate(0.1f);
				}
			}

			

			m_ImGuiLayer->Begin();
			for (Layer* layer : m_LayerStack)
			{
				layer->OnImGuiRender();
			}
			m_ImGuiLayer->End();
			
			m_Window->OnUpdate();

		}

	}

	void Application::ShutDown()
	{
	}

	void Application::OnEvent(Event& event)
	{
		EventDisPatcher dispatcher(event);
		dispatcher.Dispatcher<WindowCloseEvent>(BIND_EVENT_FUNC(Application::OnWindowClosed));
		dispatcher.Dispatcher<WindowResizeEvent>(BIND_EVENT_FUNC(Application::OnWindowResize));
		for (auto it = m_LayerStack.end(); it != m_LayerStack.begin();)
		{
			(*--it)->OnEvent(event);
			if (event.m_Handled)
				break;
		}
	}



	void Application::PushLayer(Layer* layer)
	{
		m_LayerStack.PushLayer(layer);
		layer->OnCreate();
	}
	void Application::PushOverlay(Layer* overlay)
	{
		m_LayerStack.PushOverlay(overlay);
		overlay->OnCreate();
	}
	bool Application::OnWindowClosed(WindowCloseEvent& event)
	{
		m_Active = false;
		return true;
	}
	bool Application::OnWindowResize(WindowResizeEvent& event)
	{
		if (event.GetWidth() == 0 || event.GetHeight() == 0)
		{
			m_Minimized = true;
			return false;
		}
		glViewport(0, 0, event.GetWidth(), event.GetHeight());
		m_Minimized = false;
		return false;
	}
}