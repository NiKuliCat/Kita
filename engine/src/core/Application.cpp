#include "kita_pch.h"
#include "Application.h"

#include "Log.h"
#include <GLFW/glfw3.h>

#include "Input.h"
#include "event/KeyCode.h"

#include "render/Buffer.h"
#include "render/Renderer.h"
#include "render/VulkanRenderCommand.h"

#include <filesystem>
#if __has_include("file/Project.h")
#include "file/Project.h"
#endif
namespace Kita {

	namespace
	{
		const std::string& GetDemoVertexShaderSource()
		{
			static const std::string source = R"(
struct VSInput
{
    float3 position : ATTRIBUTE0;
    float4 color : ATTRIBUTE1;
    float2 texcoords : ATTRIBUTE2;
    float3 normal : ATTRIBUTE3;
    float3 tangent : ATTRIBUTE4;
    float3 bitangent : ATTRIBUTE5;
};

struct VSOutput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

[shader("vertex")]
VSOutput main(VSInput input)
{
    VSOutput output;
    output.position = float4(input.position, 1.0);
    output.color = input.color;
    return output;
}
)";
			return source;
		}

		const std::string& GetDemoFragmentShaderSource()
		{
			static const std::string source = R"(
struct PSInput
{
    float4 position : SV_Position;
    float4 color : COLOR0;
};

[shader("fragment")]
float4 main(PSInput input) : SV_Target0
{
    return input.color;
}
)";
			return source;
		}

		Ref<Mesh> CreateFallbackDemoMesh()
		{
			std::vector<Vertex> vertices(4);

			vertices[0].position = { -0.55f, -0.45f, 0.0f };
			vertices[0].color = { 0.96f, 0.32f, 0.29f, 1.0f };
			vertices[1].position = {  0.55f, -0.45f, 0.0f };
			vertices[1].color = { 0.20f, 0.78f, 0.49f, 1.0f };
			vertices[2].position = {  0.55f,  0.45f, 0.0f };
			vertices[2].color = { 0.18f, 0.53f, 0.95f, 1.0f };
			vertices[3].position = { -0.55f,  0.45f, 0.0f };
			vertices[3].color = { 0.98f, 0.78f, 0.20f, 1.0f };

			for (auto& vertex : vertices)
			{
				vertex.texcoords = { 0.0f, 0.0f };
				vertex.normal = { 0.0f, 0.0f, 1.0f };
				vertex.tangent = { 1.0f, 0.0f, 0.0f };
				vertex.bitangent = { 0.0f, 1.0f, 0.0f };
			}

			std::vector<uint32_t> indices = { 0, 1, 2, 2, 3, 0 };
			return Mesh::Create(vertices, indices);
		}

		std::vector<std::filesystem::path> GetDemoMeshCandidatePaths()
		{
			std::vector<std::filesystem::path> candidates;

			const std::filesystem::path current = std::filesystem::current_path();
#if __has_include("file/Project.h")
			if (const Ref<Project> project = Project::GetActive())
			{
				candidates.push_back(project->GetContentDirectory() / "models" / "Sphere.fbx");
				candidates.push_back(project->GetContentDirectory() / "Sphere.fbx");
				candidates.push_back(project->GetAssetRootDirectory() / "content" / "models" / "Sphere.fbx");
			}
#endif
			candidates.push_back(current / "content" / "models" / "Sphere.fbx");
			candidates.push_back(current / ".." / "content" / "models" / "Sphere.fbx");
			candidates.push_back(current / ".." / ".." / "content" / "models" / "Sphere.fbx");
			candidates.push_back(current / "assets" / "content" / "models" / "Sphere.fbx");
			candidates.push_back(current / ".." / "assets" / "content" / "models" / "Sphere.fbx");

			return candidates;
		}

		std::vector<Ref<Mesh>> TryLoadDemoMeshesFromFile()
		{
			for (const auto& candidate : GetDemoMeshCandidatePaths())
			{
				const std::filesystem::path normalized = candidate.lexically_normal();
				if (!std::filesystem::exists(normalized))
					continue;

				std::vector<Ref<Mesh>> meshes = Mesh::LoadMeshesFromFile(normalized);
				if (!meshes.empty())
				{
					KITA_CORE_INFO("Loaded demo mesh from: {0}", normalized.string());
					return meshes;
				}
			}

			KITA_CORE_WARN("Sphere.fbx not found in expected demo locations. Falling back to procedural quad.");
			return {};
		}
	}

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
		InitDemoMeshRendering();
	}

	void Application::InitDemoMeshRendering()
	{
		KITA_CORE_ASSERT(m_VulkanContext, "Vulkan context must be initialized before demo mesh rendering");

		m_ShaderCompiler = CreateUnique<ShaderCompiler>();

		ShaderCompiler::CompileRequest vertexRequest{};
		vertexRequest.SourcePath = "VulkanDemoMesh.vert.slang";
		vertexRequest.ModuleName = "VulkanDemoMeshVertex";
		vertexRequest.EntryPointName = "main";
		vertexRequest.ShaderStage = ShaderCompiler::Stage::Vertex;

		const ShaderCompiler::CompileResult vertexResult =
			m_ShaderCompiler->CompileToSpirvFromSource(vertexRequest, GetDemoVertexShaderSource());
		KITA_CORE_ASSERT(vertexResult.Success, "Failed to compile demo vertex shader: {0}", vertexResult.Diagnostics);

		ShaderCompiler::CompileRequest fragmentRequest{};
		fragmentRequest.SourcePath = "VulkanDemoMesh.frag.slang";
		fragmentRequest.ModuleName = "VulkanDemoMeshFragment";
		fragmentRequest.EntryPointName = "main";
		fragmentRequest.ShaderStage = ShaderCompiler::Stage::Fragment;

		const ShaderCompiler::CompileResult fragmentResult =
			m_ShaderCompiler->CompileToSpirvFromSource(fragmentRequest, GetDemoFragmentShaderSource());
		KITA_CORE_ASSERT(fragmentResult.Success, "Failed to compile demo fragment shader: {0}", fragmentResult.Diagnostics);

		VulkanShader::CreateInfo vertexShaderInfo{};
		vertexShaderInfo.Name = "DemoMeshVertexShader";
		vertexShaderInfo.Stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertexShaderInfo.EntryPoint = "main";
		vertexShaderInfo.Spirv = vertexResult.Spirv;
		m_DemoVertexShader = CreateUnique<VulkanShader>(m_VulkanContext.get(), vertexShaderInfo);

		VulkanShader::CreateInfo fragmentShaderInfo{};
		fragmentShaderInfo.Name = "DemoMeshFragmentShader";
		fragmentShaderInfo.Stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragmentShaderInfo.EntryPoint = "main";
		fragmentShaderInfo.Spirv = fragmentResult.Spirv;
		m_DemoFragmentShader = CreateUnique<VulkanShader>(m_VulkanContext.get(), fragmentShaderInfo);

		m_DemoMeshes = TryLoadDemoMeshesFromFile();
		if (m_DemoMeshes.empty())
		{
			m_DemoMeshes.push_back(CreateFallbackDemoMesh());
		}

		for (const Ref<Mesh>& mesh : m_DemoMeshes)
		{
			if (!mesh)
				continue;

			mesh->CreateVulkanGeometry(*m_VulkanContext);
			KITA_CORE_ASSERT(mesh->GetVulkanGeometry(), "Failed to create demo Vulkan geometry");
		}

		VulkanGraphicsPipeline::CreateInfo pipelineInfo{};
		pipelineInfo.Name = "DemoMeshPipeline";
		pipelineInfo.VertexShader = m_DemoVertexShader.get();
		pipelineInfo.FragmentShader = m_DemoFragmentShader.get();
		pipelineInfo.Geometry = m_DemoMeshes.front()->GetVulkanGeometry();
		pipelineInfo.ColorFormat = m_VulkanContext->GetSwapchainImageFormat();
		pipelineInfo.DepthFormat = VK_FORMAT_UNDEFINED;
		pipelineInfo.Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		pipelineInfo.CullMode = VK_CULL_MODE_NONE;
		pipelineInfo.EnableDepthTest = false;
		pipelineInfo.EnableDepthWrite = false;
		pipelineInfo.EnableBlending = false;

		m_DemoPipeline = CreateUnique<VulkanGraphicsPipeline>(*m_VulkanContext, pipelineInfo);
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
				RenderDemoMesh();

				m_ImGuiLayer->End();

				VulkanRenderCommand::EndRendering(*m_VulkanContext);
				m_VulkanContext->EndFrame();
			}
			
			m_Window->OnUpdate();
		}

	}

	void Application::RenderDemoMesh()
	{
		if (!m_DemoPipeline || m_DemoMeshes.empty())
			return;

		VkCommandBuffer commandBuffer = m_VulkanContext->GetCurrentCommandBuffer();
		VulkanRenderCommand::BindPipeline(commandBuffer, *m_DemoPipeline);

		for (const Ref<Mesh>& mesh : m_DemoMeshes)
		{
			if (!mesh || !mesh->GetVulkanGeometry())
				continue;

			VulkanRenderCommand::BindGeometry(commandBuffer, *mesh->GetVulkanGeometry());
			VulkanRenderCommand::DrawGeometry(commandBuffer, *mesh->GetVulkanGeometry());
		}
	}

	void Application::ShutDown()
	{
		m_DemoPipeline.reset();
		m_DemoFragmentShader.reset();
		m_DemoVertexShader.reset();
		m_DemoMeshes.clear();
		m_ShaderCompiler.reset();
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
		m_VulkanContext->OnResize(event.GetWidth(), event.GetHeight());
		m_Minimized = false;
		return false;
	}
}
