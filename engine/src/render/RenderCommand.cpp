#include "kita_pch.h"
#include "RenderCommand.h"
#include "platform/opengl/OpenGLRendererAPI.h"
namespace Kita {

	RendererAPI* RenderCommand::s_RendererAPI = new OpenGLRendererAPI;

}