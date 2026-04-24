#pragma once
#include <glm/glm.hpp>
#include "VertexArray.h"
#include "Shader.h"
namespace Kita {

	enum class CullMode
	{
		Back,
		Front,
		None
	};

	enum class DepthTestMode
	{
		Never = 0,
		Less,
		Equal,
		Lequal,
		Greater,
		NotEqual,
		Gequal,
		Always
	};

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
		virtual void SetCullMode(CullMode cullmode) = 0;
		virtual	void SetDepthTest(bool value) = 0;
		virtual void SetDepthWrite(bool value) = 0;
		virtual void SetDepthTestMode(const DepthTestMode testMode) = 0;
		virtual void SetViewport(const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height) = 0;

		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray) = 0;
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray,const Ref<Shader>& shader) = 0;
		virtual void DrawIndexed(const Ref<VertexArray>& vertexArray, const uint32_t count) = 0;

		virtual void DrawLine(const Ref<VertexArray>& vertexArray, const Ref<Shader>& shader, const uint32_t vertexCount, const float lineWidth) = 0;
		virtual void DrawGizmoPoints(const Ref<VertexArray>& vertexArray, const Ref<Shader>& shader, const uint32_t count) = 0;

	private:
		static API s_API;

	};
}
