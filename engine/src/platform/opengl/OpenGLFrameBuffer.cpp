#include "kita_pch.h"
#include "OpenGLFrameBuffer.h"
#include "OpenGLUtil.h"
#include "core/Log.h"
namespace Kita {



	OpenGLFrameBuffer::OpenGLFrameBuffer(const FrameBufferDescriptor& descriptor)
		:m_Descriptor(descriptor)
	{
		Invalidate();
	}

	OpenGLFrameBuffer::~OpenGLFrameBuffer()
	{
		glDeleteFramebuffers(1, &m_FrameBufferID);
		glDeleteTextures(1, &m_ColorAttachment);
		glDeleteTextures(1, &m_DepthAttachment);
	}

	uint32_t OpenGLFrameBuffer::GetColorAttachment(uint32_t index) const
	{
		return m_ColorAttachment;
	}

	uint32_t OpenGLFrameBuffer::GetDepthAttachment() const
	{
		return m_DepthAttachment;
	}

	void OpenGLFrameBuffer::ReSize(uint32_t width, uint32_t height)
	{
	}

	void OpenGLFrameBuffer::Bind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, m_FrameBufferID);
		glViewport(0, 0, m_Descriptor.Width, m_Descriptor.Height);
	}

	void OpenGLFrameBuffer::UnBind()
	{
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	void OpenGLFrameBuffer::Invalidate()
	{
		glCreateFramebuffers(1, &m_FrameBufferID);

		Bind();

		glCreateTextures(GL_TEXTURE_2D, 1, &m_ColorAttachment);
		glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Descriptor.Width, m_Descriptor.Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// 可选：设置 Wrap 方式
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);

		auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER)== GL_FRAMEBUFFER_COMPLETE;
		KITA_CORE_ASSERT(status , "frame buffer is not completed");

		UnBind();

	}

	

}