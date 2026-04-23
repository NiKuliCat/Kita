#pragma once

#include "render/Texture.h"

namespace Kita {
	
	

	class OpenGLTexture : public Texture {
	public:
		OpenGLTexture(const TextureDescriptor& descriptor, const std::string& path);
		OpenGLTexture(const TextureDescriptor& descriptor, const CubemapFacePaths& faces);
		virtual ~OpenGLTexture();
		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetID() const override { return m_TextureID; }

		virtual TextureType GetType() const override { return m_TexType; }

		virtual void Bind(uint32_t slot = 3) const override;


	private:
		void  SetTextureData(int channels,int width,int height, const void* data);
		void  SetTextureCubeData(int channels,int width,int height, const std::array<const void*, 6>& facesData);
	private:
		uint32_t m_TextureID;
		uint32_t m_Width, m_Height;
		TextureDescriptor m_Descriptor;;

		TextureType m_TexType = TextureType::Texture2D;
		std::string m_Path;
	};
}