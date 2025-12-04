#pragma once
#include <glm/glm.hpp>
#include "VertexArray.h"
#include "Shader.h"
namespace Kita {
	class RendererAPI
	{
	public:
		enum class API
		{
			None = 0,
			OpenGL = 1
		};

	public:
		inline static  API GetAPI() { return s_API; }

		virtual void Clear() = 0;
		virtual void SetClearColor(const glm::vec4& color) = 0;
		virtual void SetBlend(bool value) = 0;
		virtual	void SetDepthTest(bool value) = 0;
		virtual void SetViewport(const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height) = 0;
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray) = 0;
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray,const Ref<Shader>& shader) = 0;
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, const uint32_t count) = 0;
	private:
		static API s_API;

	};
}
