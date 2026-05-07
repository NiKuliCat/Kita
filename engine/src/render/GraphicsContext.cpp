#include "kita_pch.h"
#include "GraphicsContext.h"
#include "platform/opengl/OpenGLContext.h"
#include "core/Log.h"
namespace Kita {




	Unique<GraphicsContext> GraphicsContext::Create(GLFWwindow* windowHandle, RendererAPI::API api)
	{
		switch (api)
		{
			case Kita::RendererAPI::API::None:
			{
				KITA_CORE_BREAK("unkown graphics api!");
				return nullptr;
			}
			case Kita::RendererAPI::API::OpenGL:
			{

				KITA_CORE_INFO("create opengl context");
				return  CreateUnique<OpenGLContext>(windowHandle);
			}
			case Kita::RendererAPI::API::Vulkan:
			{
				KITA_CORE_BREAK("unkown graphics api!");
				return nullptr;
			}
			default:
			{
				KITA_CORE_BREAK("unkown graphics api!");
				return nullptr;
			}
		}

	}

}