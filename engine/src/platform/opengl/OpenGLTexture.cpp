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


		struct GLTextureFormat
		{
			GLenum InternalFormat;
			GLenum DataFormat;
		};

		GLTextureFormat GetGLTextureFormat(int channels)
		{
			switch (channels)
			{
			case 4: return { GL_RGBA8, GL_RGBA };
			case 3: return { GL_RGB8,  GL_RGB };
			default: return { GL_RGB8, GL_RGB };
			}
		}
	}

	OpenGLTexture::OpenGLTexture(const TextureDescriptor& descriptor, const std::string& path)
		:m_Descriptor(descriptor), m_TexType(TextureType::Texture2D), m_Path(path)
	{
		stbi_set_flip_vertically_on_load(true);
		int width, height, channels;
		stbi_uc* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
		KITA_CORE_ASSERT(data, "Failed to load this image : {0}", path);
		
		glCreateTextures(GL_TEXTURE_2D, 1, &m_TextureID);
		SetTextureData(channels, width, height, data);

		stbi_image_free(data);

	}


	OpenGLTexture::OpenGLTexture(const TextureDescriptor& descriptor, const CubemapFacePaths& faces)
		:m_Descriptor(descriptor), m_TexType(TextureType::CubeMap)
	{
		KITA_CORE_ASSERT(faces.size() == 6, "Cubemap must have 6 faces.");

		stbi_set_flip_vertically_on_load(false);		// cubemap 通常不做垂直翻转

		std::array<stbi_uc*, 6> loaded = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

		int width, height, channels;

		for (int i = 0; i < 6; i++)
		{
			int w, h, c;
			loaded[i] = stbi_load(faces[i].c_str(), &w, &h, &c, 0);
			KITA_CORE_ASSERT(loaded[i], "Failed to load cubemap face: {0}", faces[i]);

			if (i == 0)
			{
				width = w;
				height = h;
				channels = c;
			}
			else
			{
				KITA_CORE_ASSERT(w == width && h == height, "Cubemap face size mismatch: {0}", faces[i]);
				KITA_CORE_ASSERT(c == channels, "Cubemap face channel mismatch: {0}", faces[i]);
			}

		}

		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &m_TextureID);

		std::array<const void*, 6> facePixels = {
			loaded[0], loaded[1], loaded[2], loaded[3], loaded[4], loaded[5]
		};

		SetTextureCubeData(channels, width, height, facePixels);

		for (auto* p : loaded) stbi_image_free(p);

		stbi_set_flip_vertically_on_load(true);
	}




	OpenGLTexture::~OpenGLTexture()
	{
		glDeleteTextures(1, &m_TextureID);
	}
	void OpenGLTexture::Bind(uint32_t slot) const
	{
		glBindTextureUnit(slot, m_TextureID);
	}
	void OpenGLTexture::SetTextureData(int channels, int width, int height, const void* data)
	{
		m_Width = width;
		m_Height = height;
		const uint32_t mipLevels = CalculateMipLevelCount(m_Width, m_Height);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		auto format = GetGLTextureFormat(channels);

		glTextureStorage2D(m_TextureID, mipLevels, format.InternalFormat, m_Width, m_Height);
		glTextureSubImage2D(m_TextureID, 0, 0, 0, m_Width, m_Height, format.DataFormat, GL_UNSIGNED_BYTE, data);

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

	void OpenGLTexture::SetTextureCubeData(int channels, int width, int height, const std::array<const void*, 6>& facesData)
	{

		m_Width = static_cast<uint32_t>(width);
		m_Height = static_cast<uint32_t>(height);

		const uint32_t mipLevels = m_Descriptor.EnableMipMaps ? CalculateMipLevelCount(m_Width, m_Height) : 1;
		const auto format = GetGLTextureFormat(channels);

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glTextureStorage2D(m_TextureID, mipLevels, format.InternalFormat, m_Width, m_Height);

		for (uint32_t face = 0; face < 6; ++face)
		{
			glTextureSubImage3D(
				m_TextureID,
				0,
				0, 0, static_cast<GLint>(face),
				m_Width, m_Height, 1,
				format.DataFormat,
				GL_UNSIGNED_BYTE,
				facesData[face]
			);
		}


		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

		glTextureParameteri(m_TextureID, GL_TEXTURE_MIN_FILTER, mipLevels > 1 ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
		glTextureParameteri(m_TextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_TextureID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

		if (mipLevels > 1)
		{
			glGenerateTextureMipmap(m_TextureID);
		}
	}

}
