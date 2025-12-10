#pragma once
#include <glad/glad.h>
#include "render/Shader.h"
#include <render/FrameBuffer.h>
namespace Kita {

	class OpenGLUtil
	{
	public:


		//shader
		static std::unordered_map<GLenum, std::string> GLSLReader(const std::string& filepath);
		static std::string GLSLReader(const ShaderType type, const std::string& filepath);
		static std::string GetFileNameWithoutExtension(const std::string& filepath);


		//framebuffer
		static bool IsDepthFormat(const FrameBufferTexFormat format);
		static void CreateAttachment(bool multisample, uint32_t count, uint32_t* out_id);
		static void BindAttachment(bool multisample, uint32_t id);
		
		static void AttachColorTexture(uint32_t id, int sample, GLenum internalFormat, GLenum format, uint32_t width, uint32_t height, GLenum type, int index);
		static void AttachColorTexture(FrameBufferTexFormat& format, uint32_t id, int sample, uint32_t width, uint32_t height, int index);
		static void AttachDepthTexture(uint32_t id, int samples, GLenum format, GLenum depthAttachType, uint32_t width, uint32_t height);
		static void AttachDepthTexture(FrameBufferTexFormat& format,uint32_t id, int samples,uint32_t width, uint32_t height);

	private:
		static GLenum TextureTarget(bool multisampled);

	private :
		inline static std::string Trim(const std::string& str);
	};
}