#include "kita_pch.h"
#include "VertexArray.h"
#include "RendererAPI.h"
#include "core/Log.h"
#include "platform/opengl/OpenGLVertexArray.h"
namespace Kita {

	Ref<VertexArray> VertexArray::Create()
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
		{
			return CreateRef<OpenGLVertexArray>();
			break;
		}
		case RendererAPI::API::None:
		{
			return nullptr;
			break;
		}
		}

		KITA_CORE_ERROR("UnKown RendererAPI !");
		return nullptr;
	}

}