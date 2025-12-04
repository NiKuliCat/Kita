#include "kita_pch.h"
#include "OpenGLTexture.h"
#include "third_party/stb_image/stb_image.h"
#include "core/Core.h"
#include "core/Log.h"
#include <glad/glad.h>
namespace Kita {
	OpenGLTexture::OpenGLTexture(const TextureDescriptor& descriptor, const std::string& path)
		:m_Descriptor(descriptor), m_Path(path)
	{
		stbi_set_flip_vertically_on_load(true);
		int width, height, channels;
		stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
		KITA_CORE_ASSERT(data, "Failed to load this image : {0}", path);
		m_Width = width;
		m_Height = height;

		glCreateTextures(GL_TEXTURE_2D, 1, &m_TextureID);
		glTextureStorage2D(m_TextureID, 1, GL_RGB8, m_Width, m_Height);
		glTextureSubImage2D(m_TextureID, 0, 0, 0, m_Width, m_Height, GL_RGB, GL_UNSIGNED_BYTE, data);

		glTextureParameteri(m_TextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(m_TextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glGenerateTextureMipmap(m_TextureID);
		glTextureParameteri(m_TextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

		stbi_image_free(data);

	}
	OpenGLTexture::~OpenGLTexture()
	{
		glDeleteTextures(1, &m_TextureID);
	}
	void OpenGLTexture::Bind(uint32_t slot) const
	{
		glBindTextureUnit(slot, m_TextureID);
	}
}