#pragma once
#include "RendererAPI.h"
#include "RenderCommand.h"
#include "render/UniformBuffer.h"
#include "Light.h"
#include "Texture.h"
namespace  Kita{

	struct EditorGridSettings
	{
		float CellSize = 0.25f;
		float MajorStep = 5.0f;

		float MinorWidthPx = 0.35f;
		float MajorWidthPx = 0.50f;
		float AxisWidthPx = 0.65f;

		float FadeStart = 20.0f;
		float FadeEnd = 75.0f;

		glm::vec4 MinorColor = glm::vec4(0.73f, 0.75f, 0.78f, 0.55f);
		glm::vec4 MajorColor = glm::vec4(0.86f, 0.88f, 0.89f, 0.85f);
		glm::vec4 AxisXColor = glm::vec4(0.93f, 0.35f, 0.33f, 0.60f);
		glm::vec4 AxisZColor = glm::vec4(0.33f, 0.60f, 0.96f, 0.60f); 
	};


	struct EditorGridUBOData
	{
		glm::vec4 Param0;             // x:CellSize y:MajorStep z:FadeStart w:FadeEnd
		glm::vec4 Param1;             // x:MinorWidthPx y:MajorWidthPx z:AxisWidthPx w:reserved

		glm::vec4 MinorColor;         // rgba
		glm::vec4 MajorColor;         // rgba
		glm::vec4 AxisXColor;         // rgba
		glm::vec4 AxisZColor;         // rgba
	};

	struct RenderCameraUBOData
	{
		glm::mat4 Matrix_V;
		glm::mat4 Matrix_P;
		glm::mat4 Matrix_VP;
		glm::mat4 Matrix_I_V;
		glm::mat4 Matrix_I_P;
		glm::mat4 Matrix_I_VP;
		glm::vec4 CameraPosWS;
	};


	struct RenderViewportUBOData
	{
		glm::vec4 ScreenSize;
	};

	class Renderer {


	public:
		struct SceneRenderCameraData
		{
			Ref<UniformBuffer> ViewProj_UBO;
		};

		struct ViewportRenderData
		{
			Ref<UniformBuffer> Viewport_UBO;
		};

		struct SceneRenderLightsData
		{
			Ref<UniformBuffer> MainLight_UBO;
		};

		struct EditorGridData
		{
			Ref<UniformBuffer> EditorGrid_UBO;
			Ref<VertexArray> FullScreenTriangle_VAO;
		};

		struct RenderData
		{
			SceneRenderCameraData	cameraData;
			SceneRenderLightsData	lightData;
			ViewportRenderData		screenData;
			EditorGridData			editorGridData;
		};

		static void Init();


		static void ShutDown();

		static void BeginScene(const glm::mat4& v, const glm::mat4& p, const glm::vec3& pos, const DirectLightData& mainLightData, const glm::vec2& screenSize);
		static void EndScene();


		static void Submit(const Ref<VertexArray>& vertexArray) { RenderCommand::DrawIndexed(vertexArray);}
		static void Submit(const Ref<VertexArray>& vertexArray,const Ref<Shader>& shader) { RenderCommand::DrawIndexed(vertexArray,shader);}


		static void SubmitAsLine(const Ref<VertexArray>& vertexArray, const Ref<Shader>& shader) { RenderCommand::DrawLine(vertexArray,shader);}


		static void DrawGizmoPoints(const Ref<VertexArray>& vertexArray, const Ref<Shader>& shader, const uint32_t count) { RenderCommand::DrawGizmoPoints(vertexArray, shader, count); }
		static void DrawEditorGrids(const EditorGridSettings& settings);

		static void DrawSkyBox(const Ref<Texture>& cubemap, const uint32_t slot);

	private:
		static void InitEditorGridData();

	private:
		static RenderData* m_RenderData;
	};
}