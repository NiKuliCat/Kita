#include "kita_pch.h"
#include "Renderer.h"
#include "ShaderLibrary.h"

namespace Kita {

	Renderer::RenderData* Renderer::m_RenderData = nullptr;

	void Renderer::Init()
	{
		//init scene  VBO
		m_RenderData = new RenderData();
		m_RenderData->cameraData.ViewProj_UBO = UniformBuffer::Create(sizeof(glm::mat4), 0);
		m_RenderData->lightData.MainLight_UBO = UniformBuffer::Create(sizeof(DirectLightData), 1);



		// load shader
		auto& shaderLibrary = ShaderLibrary::GetInstance();
		shaderLibrary.Load("assets/shaders/EditorDefaultShader.glsl");
		shaderLibrary.Load("assets/shaders/EditorLineShader.glsl");

	}

	void Renderer::ShutDown()
	{
		delete m_RenderData;
	}

	void Renderer::BeginScene(const glm::mat4& vp, const DirectLightData& mainLightData)
	{
		m_RenderData->cameraData.ViewProj_UBO->SetData(&vp, sizeof(glm::mat4), 0);
		m_RenderData->lightData.MainLight_UBO->SetData(&mainLightData, sizeof(DirectLightData), 0);
	}

	void Renderer::EndScene()
	{
	}

}
