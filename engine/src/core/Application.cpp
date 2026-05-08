#include "kita_pch.h"
#include "Application.h"

#include "Log.h"
#include <GLFW/glfw3.h>

#include "Input.h"
#include "event/KeyCode.h"

#include "asset/AssetManager.h"
#include "render/BufferLayout.h"
#include "render/VulkanRenderCommand.h"
#include "render/VulkanRenderTarget.h"
#include <backends/imgui_impl_vulkan.h>
namespace Kita {

	Application* Application::s_Instance = nullptr;
	Application::Application(const ApplicationDescriptor& app_descriptor)
		:m_Descriptor(app_descriptor)
	{
		s_Instance = this;
		KITA_CORE_TRACE("launch current active app: " + m_Descriptor.name);

		m_Active = true;

	}

	void Application::Run()
	{
		InitWindow();

		InitVulkanContext();

		InitImGuiLayer();
		InitRenderer();
		InitializePendingLayers();

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

	void Application::InitVulkanContext()
	{
		GLFWwindow* nativeWindow = static_cast<GLFWwindow*>(m_Window->GetNativeWindow());
		KITA_CORE_ASSERT(nativeWindow, "Application native window is null");

		m_VulkanContext = CreateUnique<VulkanContext>(nativeWindow);
		m_VulkanContext->Init();
	}

	void Application::InitImGuiLayer()
	{
		m_ImGuiLayer = new ImGuiLayer();
		KITA_CORE_TRACE("init imgui layer");
		PushOverlay(m_ImGuiLayer);
	}

	void Application::InitRenderer()
	{
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


			if (m_VulkanContext->BeginFrame())
			{
				m_ImGuiLayer->Begin();

				for (Layer* layer : m_LayerStack)
					layer->OnImGuiRender();

				VulkanRenderCommand::BeginSwapchainRendering(*m_VulkanContext, { 0.1f, 0.1f, 0.12f, 1.0f });

				m_ImGuiLayer->End();

				VulkanRenderCommand::EndRendering(*m_VulkanContext);
				m_VulkanContext->EndFrame();
			}
			
			m_Window->OnUpdate();
		}

	}

	void Application::ShutDown()
	{
		if (m_VulkanContext)
			m_VulkanContext->WaitIdle();

		m_LayerStack.DestroyAll();
		m_ImGuiLayer = nullptr;
		m_LayersInitialized = false;

		AssetManager::GetInstance().Clear();

		if (m_VulkanContext)
		{
			m_VulkanContext->WaitIdle();
			m_VulkanContext.reset();
		}

		m_Window.reset();
	}

	void Application::InitializePendingLayers()
	{
		if (m_LayersInitialized)
			return;

		if (m_ImGuiLayer && !m_ImGuiLayer->m_IsCreated)
		{
			m_ImGuiLayer->OnCreate();
			m_ImGuiLayer->m_IsCreated = true;
		}

		for (Layer* layer : m_LayerStack)
		{
			if (!layer || layer->m_IsCreated)
				continue;

			layer->OnCreate();
			layer->m_IsCreated = true;
		}

		m_LayersInitialized = true;
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
		if (m_LayersInitialized && layer && !layer->m_IsCreated)
		{
			layer->OnCreate();
			layer->m_IsCreated = true;
		}
	}
	void Application::PushOverlay(Layer* overlay)
	{
		m_LayerStack.PushOverlay(overlay);
		if (m_LayersInitialized && overlay && !overlay->m_IsCreated)
		{
			overlay->OnCreate();
			overlay->m_IsCreated = true;
		}
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
		m_VulkanContext->OnResize(event.GetWidth(), event.GetHeight());
		m_Minimized = false;
		return false;
	}
}
