#pragma once
#include "RendererAPI.h"
#include "RenderCommand.h"
#include "render/UniformBuffer.h"
#include "Light.h"
namespace  Kita{

	class Renderer {


	public:
		struct SceneRenderCameraData
		{
			Ref<UniformBuffer> ViewProj_UBO;
		};

		struct SceneRenderLightsData
		{
			Ref<UniformBuffer> MainLight_UBO;
		};

		struct RenderData
		{
			SceneRenderCameraData	cameraData;
			SceneRenderLightsData	lightData;
		};

		static void Init();
		static void ShutDown();

		static void BeginScene(const glm::mat4& vp, const DirectLightData& mainLightData);
		static void EndScene();


		static void Submit(const Ref<VertexArray>& vertexArray) { RenderCommand::DrawIndexed(vertexArray);}
		static void Submit(const Ref<VertexArray>& vertexArray,const Ref<Shader>& shader) { RenderCommand::DrawIndexed(vertexArray,shader);}


		static void SubmitAsLine(const Ref<VertexArray>& vertexArray, const Ref<Shader>& shader) { RenderCommand::DrawLine(vertexArray,shader);}



	private:
		static RenderData* m_RenderData;
	};
}