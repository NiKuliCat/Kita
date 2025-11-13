#include "kita_pch.h"
#include "Application.h"

#include "Log.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>


#include "Input.h"
#include "event/KeyCode.h"

#include "render/Buffer.h"
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
		InitImGuiLayer();

	//	RenderTest();

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

		BufferLayout layout = {
			{ShaderDataType::Float3,"position"},
			//{ShaderDataType::Float4,"color"},
			//{ShaderDataType::Float3,"normal"}
		};

		float vectices[9] = {
			-0.5f,-0.5f,0.0f,
			0.5f,-0.5f,0.0f,
			0.0f,0.5f,0.5f
		};

		uint32_t indices[3] = { 0,1,2 };



		auto vertexbuffer = VertexBuffer::Create(vectices, sizeof(vectices));
		vertexbuffer->SetLayout(layout);
		auto indexbuffer = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));

		auto vertexArray = VertexArray::Create();
		vertexArray->AddVertexBuffer(vertexbuffer);
		vertexArray->SetIndexBuffer(indexbuffer);

		while (m_Active)
		{
			glClearColor(0.12, 0.12, 0.13, 1);
			glClear(GL_COLOR_BUFFER_BIT);

			if (!m_Minimized)
			{
				for (Layer* layer : m_LayerStack)
				{
					layer->OnUpdate(0.1);
				}
			}

			vertexArray->Bind();
			glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, 0);

			m_ImGuiLayer->Begin();
			for (Layer* layer : m_LayerStack)
			{
				layer->OnImGuiRender();
			}
			m_ImGuiLayer->End();

		
			
			m_Window->OnUpdate();



			//auto [x, y] = Input::GetMousePosition();
			//KITA_CORE_TRACE("mouse position :({0},{1})", x, y);

			
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
	void Application::RenderTest()
	{
		BufferLayout layout = {
			{ShaderDataType::Float3,"position"},
			//{ShaderDataType::Float4,"color"},
			//{ShaderDataType::Float3,"normal"}
		};

		float vectices[9] = {
			-0.5f,-0.5f,0.0f,
			0.5f,-0.5f,0.0f,
			0.0f,0.5f,0.5f
		};

		uint32_t indices[3] = { 0,1,2 };



		auto vertexbuffer = VertexBuffer::Create(vectices, sizeof(vectices));
		vertexbuffer->SetLayout(layout);
		auto indexbuffer = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));

		auto vertexArray = VertexArray::Create();
		vertexArray->AddVertexBuffer(vertexbuffer);
		vertexArray->SetIndexBuffer(indexbuffer);



	}
}