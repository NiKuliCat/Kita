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

		RenderTest();

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
			{ShaderDataType::Float4,"color"},
			{ShaderDataType::Float2,"texcoords"}
		};

		BufferLayout boxLayout = {
			{ShaderDataType::Float3,"position"},
			{ShaderDataType::Float4,"color"},
			{ShaderDataType::Float2,"texcoord"},
			{ShaderDataType::Float3,"normal"},
			{ShaderDataType::Float3,"tangent"},
			{ShaderDataType::Float3,"bitangent"}
		};

		float vectices[36] = {
			-0.5f,-0.5f,0.0f,1.0f,0.0f,1.0f,1.0f,0.0f,0.0f,
			-0.5f,0.5f,0.0f,0.0f,1.0f,0.0f,1.0f,0.0f,1.0f,
			0.5f,0.5f,0.0f,0.0f,0.0f,1.0f,1.0f,1.0f,1.0f,
			0.5f,-0.5f,0.0f,1.0f,0.0f,0.0f,1.0f,1.0f,0.0f
		};

		uint32_t indices[6] = { 0,2,1,0,3,2};

		auto shader = Shader::Create("assets/shaders/EditorDefaultShader.glsl");

		auto object = new Object("testObject");
		object->LoadMeshs("assets/models/box.fbx");

		auto boxVertexBuffer = VertexBuffer::Create((float*)object->GetMeshs()[0]->GetVertices().data(), sizeof(Vertex) * object->GetMeshs()[0]->GetVertices().size());
		auto boxIndexBuffer = IndexBuffer::Create(object->GetMeshs()[0]->GetIndices().data(), object->GetMeshs()[0]->GetIndices().size());
		boxVertexBuffer->SetLayout(boxLayout);



		auto vertexbuffer = VertexBuffer::Create(vectices, sizeof(vectices));
		vertexbuffer->SetLayout(layout);
		auto indexbuffer = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));



		auto vertexArray = VertexArray::Create();
		vertexArray->AddVertexBuffer(vertexbuffer);
		vertexArray->SetIndexBuffer(indexbuffer);

		
		auto camera = new OrthographicCamera(1.0f, m_Descriptor.width / (float)m_Descriptor.height, -1.0f, 1.0f);

		TextureDescriptor texDesc{};
		auto texture = Texture::Create(texDesc, "assets/textures/test.jpg");

		texture->Bind(0);
		shader->SetInt("MainTex", 0);
		auto uniformBuffer = UniformBuffer::Create(sizeof(glm::mat4), 0);
		
		uniformBuffer->SetData(&camera->GetProjectionMatrix(), sizeof(glm::mat4), 0);

		while (m_Active)
		{
			RenderCommand::SetDepthTest(true);
			RenderCommand::SetBlend(true);
			RenderCommand::SetCullMode(RendererAPI::CullMode::Back);
			RenderCommand::SetClearColor(glm::vec4(0.12, 0.12, 0.13, 1));
			RenderCommand::Clear();

			if (!m_Minimized)
			{
				for (Layer* layer : m_LayerStack)
				{
					layer->OnUpdate(0.1f);
				}
			}

			Renderer::Submit(vertexArray, shader);

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
	void Application::RenderTest()
	{

		
	}
}