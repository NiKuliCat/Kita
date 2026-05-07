#pragma once
#include "RendererAPI.h"


namespace Kita {


	class RenderCommand
	{
	public:
		inline static void Clear()
		{
			throw std::runtime_error("Legacy RenderCommand::Clear path is disabled. Use VulkanRenderCommand instead.");
		}
		inline static void SetClearColor(const glm::vec4 color)
		{
			(void)color;
			throw std::runtime_error("Legacy RenderCommand::SetClearColor path is disabled. Use VulkanRenderCommand instead.");
		}
		inline static void SetBlend(bool enable)
		{
			(void)enable;
			throw std::runtime_error("Legacy RenderCommand::SetBlend path is disabled.");
		}
		inline static void SetCullMode(CullMode cullmode)
		{
			(void)cullmode;
			throw std::runtime_error("Legacy RenderCommand::SetCullMode path is disabled.");
		}
		inline static void SetDepthTest(bool enable)
		{
			(void)enable;
			throw std::runtime_error("Legacy RenderCommand::SetDepthTest path is disabled.");
		}
		inline static void SetDepthWrite(bool enable)
		{
			(void)enable;
			throw std::runtime_error("Legacy RenderCommand::SetDepthWrite path is disabled.");
		}
		inline static void SetDepthTestMode(DepthTestMode testmode)
		{
			(void)testmode;
			throw std::runtime_error("Legacy RenderCommand::SetDepthTestMode path is disabled.");
		}
		inline static void SetColorAttachmentWriteMask(const std::vector<bool>& enabledAttachments)
		{
			(void)enabledAttachments;
			throw std::runtime_error("Legacy RenderCommand::SetColorAttachmentWriteMask path is disabled.");
		}
		inline static void DrawIndexed(const Ref<VertexArray>& vertexArray)
		{
			(void)vertexArray;
			throw std::runtime_error("Legacy RenderCommand::DrawIndexed path is disabled.");
		}
		inline static void DrawIndexed(const Ref<VertexArray>& vertexArray,const Ref<Shader>& shader)
		{
			(void)vertexArray;
			(void)shader;
			throw std::runtime_error("Legacy RenderCommand::DrawIndexed(shader) path is disabled.");
		}
		inline static void DrawIndexed(const Ref<VertexArray>& vertexArray, const uint32_t count)
		{
			(void)vertexArray;
			(void)count;
			throw std::runtime_error("Legacy RenderCommand::DrawIndexed(count) path is disabled.");
		}

		inline static void DrawLine(const Ref<VertexArray>& vertexArray,const Ref<Shader>& shader, const uint32_t vertexCount, const float lineWidth)
		{
			(void)vertexArray;
			(void)shader;
			(void)vertexCount;
			(void)lineWidth;
			throw std::runtime_error("Legacy RenderCommand::DrawLine path is disabled.");
		}

		inline static void DrawGizmoPoints(const Ref<VertexArray>& vertexArray, const Ref<Shader>& shader , const uint32_t count)
		{
			(void)vertexArray;
			(void)shader;
			(void)count;
			throw std::runtime_error("Legacy RenderCommand::DrawGizmoPoints path is disabled.");
		}



		inline static void  SetViewport(const uint32_t x, const  uint32_t y, const uint32_t width, const uint32_t height)
		{
			(void)x;
			(void)y;
			(void)width;
			(void)height;
			throw std::runtime_error("Legacy RenderCommand::SetViewport path is disabled.");
		}
	};
}
