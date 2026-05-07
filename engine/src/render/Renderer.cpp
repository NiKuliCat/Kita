#include "kita_pch.h"
#include "Renderer.h"
#include "RenderAssetUtils.h"
#include "scene/Gizmo.h"
namespace Kita {


	// 切换到 vulkan

	Renderer::RenderData* Renderer::m_RenderData = nullptr;

	void Renderer::Init()
	{
		//init scene  VBO
		m_RenderData = new RenderData();
		m_RenderData->cameraData.ViewProj_UBO = UniformBuffer::Create(sizeof(RenderCameraUBOData), 0);
		m_RenderData->lightData.MainLight_UBO = UniformBuffer::Create(sizeof(DirectLightData), 1);
		m_RenderData->screenData.Viewport_UBO = UniformBuffer::Create(sizeof(RenderViewportUBOData), 2);



		InitEditorGridData();
		Gizmo::Init();
	}

	void Renderer::ShutDown()
	{
		delete m_RenderData;
	}

	void Renderer::BeginScene(const glm::mat4& v, const glm::mat4& p,const glm::vec3& pos,const DirectLightData& mainLightData,const glm::vec2& screenSize)
	{
		RenderCameraUBOData cameraData{};
		cameraData.Matrix_V = v;
		cameraData.Matrix_P = p;
		cameraData.Matrix_I_V = glm::inverse(v);
		cameraData.Matrix_I_P = glm::inverse(p);
		cameraData.Matrix_VP = p * v;
		cameraData.Matrix_I_VP = glm::inverse(cameraData.Matrix_VP);
		cameraData.CameraPosWS = glm::vec4(pos, 0.0f);


		RenderViewportUBOData screenData{};
		screenData.ScreenSize = glm::vec4(screenSize.x, screenSize.y, 1 / screenSize.x, 1 / screenSize.y);


		m_RenderData->cameraData.ViewProj_UBO->SetData(&cameraData, sizeof(RenderCameraUBOData), 0);
		m_RenderData->lightData.MainLight_UBO->SetData(&mainLightData, sizeof(DirectLightData), 0);
		m_RenderData->screenData.Viewport_UBO->SetData(&screenData, sizeof(RenderViewportUBOData), 0);
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

		auto gridShader = RenderAssetUtils::GetRuntimeShader("packages/render/shaders/EditorGridShader.glsl");
		if (!gridShader)
		{
			return;
		}
	

		RenderCommand::SetCullMode(CullMode::None);
		RenderCommand::SetDepthTest(true);
		RenderCommand::SetDepthWrite(false);
		RenderCommand::SetBlend(true);
		RenderCommand::SetColorAttachmentWriteMask({ true, false, false });

		Submit(m_RenderData->editorGridData.FullScreenTriangle_VAO, gridShader);
		RenderCommand::SetColorAttachmentWriteMask({ true, true, true });
		RenderCommand::SetDepthWrite(true);
	}

	void Renderer::DrawSkyBox(const Ref<Texture>& cubemap, const uint32_t slot)
	{
		if (!cubemap)
		{
			return;
		}

		auto skyboxShader = RenderAssetUtils::GetRuntimeShader("packages/render/shaders/DefaultSkyBox.glsl");
		if (!skyboxShader)
		{
			return;
		}

		RenderCommand::SetDepthWrite(false);
		RenderCommand::SetDepthTestMode(DepthTestMode::Lequal);

		cubemap->Bind(slot);
		skyboxShader->SetInt("SkyboxTex", slot);

		Submit(m_RenderData->editorGridData.FullScreenTriangle_VAO, skyboxShader);

		RenderCommand::SetDepthTestMode(DepthTestMode::Less);
		RenderCommand::SetDepthWrite(true);
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
