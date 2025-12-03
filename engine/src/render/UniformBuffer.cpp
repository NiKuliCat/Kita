#include "kita_pch.h"
#include "UniformBuffer.h"
#include <glad/glad.h>
#include "RendererAPI.h"
#include "core/Log.h"
#include "platform/opengl/OpenGLUniformBuffer.h"
namespace Kita {
	Ref<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
		{
			return CreateRef<OpenGLUniformBuffer>(size,binding);
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