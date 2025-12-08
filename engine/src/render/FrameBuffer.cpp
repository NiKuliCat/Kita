#include "kita_pch.h"
#include "FrameBuffer.h"
#include "RendererAPI.h"
#include "platform/opengl/OpenGLFrameBuffer.h"
#include "core/Log.h"
namespace Kita {

	Ref<FrameBuffer> FrameBuffer::Create(const FrameBufferDescriptor& descriptor)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
		{
			return CreateRef<OpenGLFrameBuffer>(descriptor);
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