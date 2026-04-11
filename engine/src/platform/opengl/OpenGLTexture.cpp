#include "kita_pch.h"
#include "OpenGLTexture.h"
#include "third_party/stb_image/stb_image.h"
#include "core/Core.h"
#include "core/Log.h"
#include <algorithm>
#include <glad/glad.h>
namespace Kita {
	namespace {
		uint32_t CalculateMipLevelCount(uint32_t width, uint32_t height)
		{
			uint32_t levels = 1;
			uint32_t size = std::max(width, height);
			while (size > 1)
			{
				size /= 2;
				levels++;
			}
			return levels;
		}
	}

	OpenGLTexture::OpenGLTexture(const TextureDescriptor& descriptor, const std::string& path)
		:m_Descriptor(descriptor), m_Path(path)
	{
		stbi_set_flip_vertically_on_load(true);
		int width, height, channels;
		stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
		KITA_CORE_ASSERT(data, "Failed to load this image : {0}", path);
		
		glCreateTextures(GL_TEXTURE_2D, 1, &m_TextureID);
		SetTextureData(channels, width, height, data);

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
	void OpenGLTexture::SetTextureData(int channels, int width, int height, void* data)
	{
		m_Width = width;
		m_Height = height;
		const uint32_t mipLevels = CalculateMipLevelCount(m_Width, m_Height);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		switch (channels)
		{
			case 3:
			{
				glTextureStorage2D(m_TextureID, mipLevels, GL_RGB8, m_Width, m_Height);
				glTextureSubImage2D(m_TextureID, 0, 0, 0, m_Width, m_Height, GL_RGB, GL_UNSIGNED_BYTE, data);
				break;
			}
			case 4:
			{
				glTextureStorage2D(m_TextureID, mipLevels, GL_RGBA8, m_Width, m_Height);
				glTextureSubImage2D(m_TextureID, 0, 0, 0, m_Width, m_Height, GL_RGBA, GL_UNSIGNED_BYTE, data);
				break;
			}
			default:
			{
				glTextureStorage2D(m_TextureID, mipLevels, GL_RGB8, m_Width, m_Height);
				glTextureSubImage2D(m_TextureID, 0, 0, 0, m_Width, m_Height, GL_RGB, GL_UNSIGNED_BYTE, data);
				break;
			}
		}
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
		glTextureParameteri(m_TextureID, GL_TEXTURE_MIN_FILTER, mipLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		glTextureParameteri(m_TextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
		if (mipLevels > 1)
		{
			glGenerateTextureMipmap(m_TextureID);
		}
		// Store the texture in GPU memory and upload the source pixels as RGB/RGBA data.
		
	}

}
