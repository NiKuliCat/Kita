#include "renderer_pch.h"
#include "EditorLayer.h"
#include "imgui.h"

#include <glm/glm.hpp>
namespace Kita {
	EditorLayer::EditorLayer()
		:Layer("EditorLayer")
	{
		m_Camera = new OrthographicCamera(0.5f, 16.0f / 9.0f, -1.0f, 1.0f);
	}
	void EditorLayer::OnCreate()
	{
		float vectices[36] = {
		-0.5f,-0.5f,0.0f,1.0f,0.0f,1.0f,1.0f,0.0f,0.0f,
		-0.5f,0.5f,0.0f,0.0f,1.0f,0.0f,1.0f,0.0f,1.0f,
		0.5f,0.5f,0.0f,0.0f,0.0f,1.0f,1.0f,1.0f,1.0f,
		0.5f,-0.5f,0.0f,1.0f,0.0f,0.0f,1.0f,1.0f,0.0f
		};

		uint32_t indices[6] = { 0,2,1,0,3,2 };

		BufferLayout layout = {
			{ShaderDataType::Float3,"position"},
			{ShaderDataType::Float4,"color"},
			{ShaderDataType::Float2,"texcoords"}
		};

		auto shaderLibrary = ShaderLibrary::GetInstance();
		shaderLibrary.Load("assets/shaders/EditorDefaultShader.glsl");
		m_Shader = shaderLibrary.Get("EditorDefaultShader");

		

		auto vertexbuffer = VertexBuffer::Create(vectices, sizeof(vectices));
		vertexbuffer->SetLayout(layout);
		auto indexbuffer = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));

		m_VertexArray = VertexArray::Create();
		m_VertexArray->AddVertexBuffer(vertexbuffer);
		m_VertexArray->SetIndexBuffer(indexbuffer);

		TextureDescriptor texDesc{};
		m_Texture = Texture::Create(texDesc, "assets/textures/test.jpg");
		m_Texture->Bind(3);
		m_Shader->SetInt("MainTex", 3);
		m_UniformBuffer = UniformBuffer::Create(sizeof(glm::mat4), 0);

		m_UniformBuffer->SetData(&m_Camera->GetProjectionMatrix(), sizeof(glm::mat4), 0);


		FrameBufferDescriptor disc;
		disc.Width = 1280;
		disc.Height = 720;

		m_FrameBuffer = FrameBuffer::Create(disc);
		m_SceneTexID = m_FrameBuffer->GetColorAttachment(0);
		RenderCommand::SetDepthTest(true);
		RenderCommand::SetBlend(true);
		RenderCommand::SetCullMode(RendererAPI::CullMode::Back);

	}

	void EditorLayer::OnUpdate(float daltaTime)
	{
		m_FrameBuffer->Bind();
		RenderCommand::SetClearColor(glm::vec4(0.12, 0.12, 0.13, 1));
		RenderCommand::Clear();
		Renderer::Submit(m_VertexArray,m_Shader);
		m_FrameBuffer->UnBind();
	}

	void EditorLayer::OnDestroy()
	{
	}

	void EditorLayer::OnImGuiRender()
	{
		static bool p_open = true;
		static bool opt_fullscreen = true;
		static bool opt_padding = false;
		static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

		if (opt_fullscreen)
		{
			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		}
		else
		{
			dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
		}

		if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
			window_flags |= ImGuiWindowFlags_NoBackground;

		if (!opt_padding)
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

		ImGui::Begin("DockSpace Demo", &p_open, window_flags);

		if (!opt_padding)
			ImGui::PopStyleVar();

		if (opt_fullscreen)
			ImGui::PopStyleVar(2);

		ImGuiIO& io = ImGui::GetIO();
		ImGuiStyle& style = ImGui::GetStyle();
		ImVec2 minWindowSize = style.WindowMinSize;

		if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
		{
			ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
			ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
		}
		style.WindowMinSize = minWindowSize;



		ImGui::ShowDemoWindow();

		ImGui::Begin("Test Window");
		ImGui::Text("Hello from another window!");
		ImGui::End();


		ImGui::Begin("Viewport");
		uint32_t ScreenRT_ID = m_FrameBuffer->GetColorAttachment(0);
		ImGui::Image(ScreenRT_ID, ImVec2{1280,720 }, ImVec2(0, 1), ImVec2(1, 0));
		ImGui::End();

		ImGui::End();


	}

	void EditorLayer::OnEvent(Event& event)
	{
	}

}
