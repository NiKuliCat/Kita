#include "kita_pch.h"
#include "Texture.h"
#include "RendererAPI.h"
#include "core/Log.h"
#include "platform/opengl/OpenGLTexture.h"
namespace Kita {
	Ref<Texture> Texture::Create(const TextureDescriptor& descriptor, const std::string& path)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
		{
			return CreateRef<OpenGLTexture>(descriptor,path);
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
