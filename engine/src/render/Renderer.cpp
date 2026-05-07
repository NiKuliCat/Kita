#include "kita_pch.h"
#include "Renderer.h"

namespace Kita {

	Renderer::RenderData* Renderer::m_RenderData = nullptr;

	void Renderer::Init()
	{
		throw std::runtime_error("Legacy Renderer::Init path is disabled during Vulkan-only migration.");
	}

	void Renderer::ShutDown()
	{
		m_RenderData = nullptr;
	}

	void Renderer::BeginScene(const glm::mat4& v, const glm::mat4& p, const glm::vec3& pos, const DirectLightData& mainLightData, const glm::vec2& screenSize)
	{
		(void)v;
		(void)p;
		(void)pos;
		(void)mainLightData;
		(void)screenSize;
		throw std::runtime_error("Legacy Renderer::BeginScene path is disabled during Vulkan-only migration.");
	}

	void Renderer::EndScene()
	{
		throw std::runtime_error("Legacy Renderer::EndScene path is disabled during Vulkan-only migration.");
	}

	void Renderer::DrawEditorGrids(const EditorGridSettings& settings)
	{
		(void)settings;
		throw std::runtime_error("Legacy Renderer::DrawEditorGrids path is disabled during Vulkan-only migration.");
	}

	void Renderer::DrawSkyBox(const Ref<Texture>& cubemap, const uint32_t slot)
	{
		(void)cubemap;
		(void)slot;
		throw std::runtime_error("Legacy Renderer::DrawSkyBox path is disabled during Vulkan-only migration.");
	}

	void Renderer::InitEditorGridData()
	{
		throw std::runtime_error("Legacy Renderer::InitEditorGridData path is disabled during Vulkan-only migration.");
	}

}
