#pragma once
#include "render/RendererAPI.h"


namespace Kita {


	class OpenGLRendererAPI : public RendererAPI
	{
	public:
		virtual void Clear() override;
		virtual void SetClearColor(const glm::vec4& color) override;
		virtual void SetBlend(bool value) override;
		virtual	void SetDepthTest(bool value) override;
		virtual void SetViewport(const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height) override;
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray) override;
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray,const Ref<Shader>& shader) override;
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, const uint32_t count) override;
	};
}