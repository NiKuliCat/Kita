#include "renderer_pch.h"
#include "EditorLayer.h"
#include "imgui.h"

#include <glm/glm.hpp>
#include <imgui_internal.h>
#include <glm/gtc/type_ptr.hpp>
namespace Kita {

	EditorLayer::EditorLayer()
		:Layer("EditorLayer")
	{
		m_Camera = new PerspectiveCamera(45.0f, 16.0f / 9.0f, 0.1f, 100.0f);
	
		m_CameraTransform = Transform();
		m_ObjTransform = Transform();

		m_ObjTransform.SetPosition({ 0.0f,0.0f,-2.0f });
		m_CameraTransform.SetPosition({ 0.0f,0.0f,-5.0f });
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

		BufferLayout boxLayout = {
			{ShaderDataType::Float3,"position"},
			{ShaderDataType::Float4,"color"},
			{ShaderDataType::Float2,"texcoords"},
			{ShaderDataType::Float3,"normal"},
			{ShaderDataType::Float3,"tangent"},
			{ShaderDataType::Float3,"bitangent"}
		};


		m_Object = CreateRef<Object>("TestObject");
		m_Object->LoadMeshs("assets/models/box.fbx");

		auto boxVectices = m_Object->GetMeshs()[0]->GetVertices();
		auto boxIndices = m_Object->GetMeshs()[0]->GetIndices();

		auto vertexbuffer = VertexBuffer::Create(boxVectices.data(), sizeof(Vertex) * boxVectices.size());
		vertexbuffer->SetLayout(boxLayout);
		auto indexbuffer = IndexBuffer::Create(boxIndices.data(), boxIndices.size());

	/*	auto vertexbuffer = VertexBuffer::Create(vectices, sizeof(vectices));
		vertexbuffer->SetLayout(layout);
		auto indexbuffer = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));*/

		m_VertexArray = VertexArray::Create();
		m_VertexArray->AddVertexBuffer(vertexbuffer);
		m_VertexArray->SetIndexBuffer(indexbuffer);



		auto shaderLibrary = ShaderLibrary::GetInstance();
		shaderLibrary.Load("assets/shaders/EditorDefaultShader.glsl");
		m_Shader = shaderLibrary.Get("EditorDefaultShader");

		

		

		TextureDescriptor texDesc{};
		m_Texture = Texture::Create(texDesc, "assets/textures/test.jpg");
		m_Texture->Bind(3);
		m_Shader->SetInt("MainTex", 3);
		m_VPUniformBuffer = UniformBuffer::Create(sizeof(glm::mat4), 0);

	


		FrameBufferDescriptor disc;
		disc.Width = 1280;
		disc.Height = 720;

		m_ViewportSize = { 1280,720 };

		m_FrameBuffer = FrameBuffer::Create(disc);
		m_SceneTexID = m_FrameBuffer->GetColorAttachment(0);
		RenderCommand::SetDepthTest(true);
		RenderCommand::SetBlend(true);
		RenderCommand::SetCullMode(RendererAPI::CullMode::Back);

	}

	void EditorLayer::OnUpdate(float daltaTime)
	{
	
		glm::mat4 vp = m_Camera->GetProjectionMatrix() * m_CameraTransform.GetViewMatrix() ;
		glm::mat4 m = m_ObjTransform.GetTransformMatrix();
		m_VPUniformBuffer->SetData(&vp, sizeof(glm::mat4), 0);
		m_Shader->SetMat4("Model", m);
		auto desc = m_FrameBuffer->GetDescriptor();
		if (m_ViewportSize.x != desc.Width || m_ViewportSize.y != desc.Height)
		{
			m_FrameBuffer->ReSize((uint32_t)m_ViewportSize.x, (uint32_t)m_ViewportSize.y);
			m_Camera->SetAspectRatio(m_ViewportSize.x / m_ViewportSize.y);
			auto vp = m_Camera->GetProjectionMatrix() * m_CameraTransform.GetViewMatrix();
			m_VPUniformBuffer->SetData(&vp, sizeof(glm::mat4), 0);
			KITA_CLENT_TRACE("Resize FrameBuffer to : {0},{1}", m_ViewportSize.x, m_ViewportSize.y);
		}
		
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
		ImGui::DragFloat3("##position", glm::value_ptr(m_CameraTransform.GetPosition()), 0.04f, 0.0f, 0.0f, "%.2f");
		ImGui::DragFloat3("##rotate", glm::value_ptr(m_CameraTransform.GetRotation()), 0.04f, 0.0f, 0.0f, "%.2f");
		ImGui::DragFloat3("##scale", glm::value_ptr(m_CameraTransform.GetScale()), 0.04f, 0.0f, 0.0f, "%.2f");
		ImGui::PushID(1);
		ImGui::DragFloat3("##position", glm::value_ptr(m_ObjTransform.GetPosition()), 0.04f, 0.0f, 0.0f, "%.2f");
		ImGui::DragFloat3("##rotate", glm::value_ptr(m_ObjTransform.GetRotation()), 0.04f, 0.0f, 0.0f, "%.2f");
		ImGui::DragFloat3("##scale", glm::value_ptr(m_ObjTransform.GetScale()), 0.04f, 0.0f, 0.0f, "%.2f");
		ImGui::PopID();
		ImGui::End();


		ImGui::Begin("Viewport");
		uint32_t ScreenRT_ID = m_FrameBuffer->GetColorAttachment(0);
		{
			ImVec2 ViewportSize = ImGui::GetContentRegionAvail();
			m_ViewportSize = { ViewportSize.x,ViewportSize.y };
		}
		ImGui::Image(ScreenRT_ID, ImVec2{ m_ViewportSize.x,m_ViewportSize.y }, ImVec2(0, 1), ImVec2(1, 0));

		ImGui::End();


		ImGui::End();


	}

	void EditorLayer::OnEvent(Event& event)
	{
	}

	

}
