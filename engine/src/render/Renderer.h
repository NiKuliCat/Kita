#pragma once
#include "RendererAPI.h"
#include "RenderCommand.h"
namespace  Kita{

	class Renderer {


	public:



		static void Submit(const Ref<VertexArray>& vertexArray) { RenderCommand::DrawIndexed(vertexArray);}
		static void Submit(const Ref<VertexArray>& vertexArray,const Ref<Shader>& shader) { RenderCommand::DrawIndexed(vertexArray,shader);}
	};
}