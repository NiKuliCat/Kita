#include "kita_pch.h"
#include "Renderer.h"
#include "ShaderLibrary.h"

namespace Kita {

	Renderer::RenderData* Renderer::m_RenderData = nullptr;

	void Renderer::Init()
	{
		//init scene  VBO
		m_RenderData = new RenderData();
		m_RenderData->cameraData.ViewProj_UBO = UniformBuffer::Create(sizeof(RenderCameraUBOData), 0);
		m_RenderData->lightData.MainLight_UBO = UniformBuffer::Create(sizeof(DirectLightData), 1);


		InitEditorGridData();
		// load shader
		auto& shaderLibrary = ShaderLibrary::GetInstance();
		shaderLibrary.Load("assets/shaders/EditorDefaultShader.glsl");
		shaderLibrary.Load("assets/shaders/EditorLineShader.glsl");
		shaderLibrary.Load("assets/shaders/EditorGridShader.glsl");
	}

	void Renderer::ShutDown()
	{
		delete m_RenderData;
	}

	void Renderer::BeginScene(const glm::mat4& v, const glm::mat4& p,const glm::vec3& pos,const DirectLightData& mainLightData)
	{
		auto& cameraData = RenderCameraUBOData{};
		cameraData.Matrix_V = v;
		cameraData.Matrix_P = p;
		cameraData.Matrix_I_V = glm::inverse(v);
		cameraData.Matrix_I_P = glm::inverse(p);
		cameraData.Matrix_VP = p * v;
		cameraData.Matrix_I_VP = glm::inverse(cameraData.Matrix_VP);
		cameraData.CameraPosWS = glm::vec4(pos, 0.0f);

		m_RenderData->cameraData.ViewProj_UBO->SetData(&cameraData, sizeof(RenderCameraUBOData), 0);
		m_RenderData->lightData.MainLight_UBO->SetData(&mainLightData, sizeof(DirectLightData), 0);
	}

	void Renderer::EndScene()
	{
	}

	void Renderer::DrawEditorGrids(const EditorGridSettings& settings)
	{
		EditorGridUBOData ubo{};
		ubo.Param0 = glm::vec4(
			std::max(settings.CellSize, 0.0001f),
			std::max(settings.MajorStep, 1.0f),
			settings.FadeStart,
			std::max(settings.FadeEnd, settings.FadeStart + 0.001f));
		ubo.Param1 = glm::vec4(settings.MinorWidthPx,settings.MajorWidthPx, settings.AxisWidthPx, 0.0f); // 线宽像素
		ubo.MinorColor = settings.MinorColor;
		ubo.MajorColor = settings.MajorColor;
		ubo.AxisXColor = settings.AxisXColor;
		ubo.AxisZColor = settings.AxisZColor;

		m_RenderData->editorGridData.EditorGrid_UBO->SetData(&ubo, sizeof(EditorGridUBOData), 0);

		auto& shaderlib = ShaderLibrary::GetInstance();
		auto gridShader = shaderlib.Get("EditorGridShader");
	

		RenderCommand::SetCullMode(RendererAPI::CullMode::None);
		RenderCommand::SetDepthTest(true);
		RenderCommand::SetBlend(true);

		Submit(m_RenderData->editorGridData.FullScreenTriangle_VAO, gridShader);
	}

	void Renderer::InitEditorGridData()
	{
		float verts[9] = {
			-1.0f, -1.0f, 0.0f,
			 3.0f, -1.0f, 0.0f,
			-1.0f,  3.0f, 0.0f
		};

		uint32_t  indices[3] = { 0, 1, 2 };

		BufferLayout fullScreenTriangleLayout = {
			{ShaderDataType::Float3,"position"}
		};
		auto vbo = VertexBuffer::Create(verts, sizeof(verts));
		vbo->SetLayout(fullScreenTriangleLayout);
		auto ibo = IndexBuffer::Create(indices, sizeof(indices) / sizeof(uint32_t));

		m_RenderData->editorGridData.FullScreenTriangle_VAO = VertexArray::Create();
		m_RenderData->editorGridData.FullScreenTriangle_VAO->AddVertexBuffer(vbo);
		m_RenderData->editorGridData.FullScreenTriangle_VAO->SetIndexBuffer(ibo);

		m_RenderData->editorGridData.EditorGrid_UBO = UniformBuffer::Create(sizeof(EditorGridUBOData), 10);


	}

}
