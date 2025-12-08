#pragma once

#include "render/Texture.h"

namespace Kita {
	
	

	class OpenGLTexture : public Texture {
	public:
		OpenGLTexture(const TextureDescriptor& descriptor, const std::string& path);
		virtual ~OpenGLTexture();
		virtual uint32_t GetWidth() const override { return m_Width; }
		virtual uint32_t GetHeight() const override { return m_Height; }
		virtual uint32_t GetID() const override { return m_TextureID; }

		virtual void Bind(uint32_t slot = 3) const override;


	private:
		void  SetTextureData(int channels,int width,int height,void* data);
	private:
		uint32_t m_TextureID;
		uint32_t m_Width, m_Height;
		TextureDescriptor m_Descriptor;;
		std::string m_Path;
	};
}