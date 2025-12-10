#include "kita_pch.h"
#include "OpenGLFrameBuffer.h"
#include "OpenGLUtil.h"
#include "core/Log.h"
namespace Kita {



	OpenGLFrameBuffer::OpenGLFrameBuffer(const FrameBufferDescriptor& descriptor)
		:m_Descriptor(descriptor)
	{
		for (auto desc : m_Descriptor.AttachmentsDescription.AttachmentsDesc)
		{
			if (OpenGLUtil::IsDepthFormat(desc.Format))
			{
				m_DepthAttachmentDesc = desc.Format;
			}
			else
			{
				m_ColorAttachmentsDesc.push_back(desc.Format);
			}

		}
		Invalidate();
	}

	OpenGLFrameBuffer::~OpenGLFrameBuffer()
	{
		glDeleteFramebuffers(1, &m_FrameBufferID);
		glDeleteTextures(m_ColorAttachments.size(), m_ColorAttachments.data());
		glDeleteTextures(1, &m_DepthAttachment);
	}

	uint32_t OpenGLFrameBuffer::GetColorAttachment(uint32_t index) const
	{
		return m_ColorAttachments[index];
	}

	uint32_t OpenGLFrameBuffer::GetDepthAttachment() const
	{
		return m_DepthAttachment;
	}

	void OpenGLFrameBuffer::ReSize(uint32_t width, uint32_t height)
	{
		m_Descriptor.Width = width;
		m_Descriptor.Height = height;

		Invalidate();
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

		if (m_FrameBufferID)
		{
			glDeleteFramebuffers(1, &m_FrameBufferID);
			glDeleteTextures(m_ColorAttachments.size(), m_ColorAttachments.data());
			glDeleteTextures(1, &m_DepthAttachment);

			m_ColorAttachments.clear();
			m_DepthAttachment = 0;
		}
		glCreateFramebuffers(1, &m_FrameBufferID);

		Bind();
		bool multisampled = m_Descriptor.Samples > 1;
		if (m_ColorAttachmentsDesc.size() > 0)
		{
			m_ColorAttachments.resize(m_ColorAttachmentsDesc.size());
			OpenGLUtil::CreateAttachment(multisampled, m_ColorAttachments.size(), m_ColorAttachments.data());
			for (size_t i = 0; i < m_ColorAttachments.size(); i++)
			{
				OpenGLUtil::BindAttachment(multisampled, m_ColorAttachments[i]);
				OpenGLUtil::AttachColorTexture(m_ColorAttachmentsDesc[i].Format, m_ColorAttachments[i],m_Descriptor.Samples, m_Descriptor.Width, m_Descriptor.Height, i);
			}
		}

		if (m_DepthAttachmentDesc.Format != FrameBufferTexFormat::None)
		{
			OpenGLUtil::CreateAttachment(multisampled, 1, &m_DepthAttachment);
			OpenGLUtil::BindAttachment(multisampled, m_DepthAttachment);
			OpenGLUtil::AttachDepthTexture(m_DepthAttachmentDesc.Format, m_DepthAttachment, m_Descriptor.Samples, m_Descriptor.Width, m_Descriptor.Height);
		}

		if (m_ColorAttachments.size() > 1)
		{
			KITA_CORE_ASSERT(m_ColorAttachments.size() >= 4, "OpenGL frame buffer size is more than 4 ");
			GLenum buffers[2] = { GL_COLOR_ATTACHMENT0,GL_COLOR_ATTACHMENT1 };
			glDrawBuffers(m_ColorAttachments.size(), buffers);
		}
		else if (m_ColorAttachments.empty())
		{
			//only depth pass
			glDrawBuffer(GL_NONE);
		}
		auto s = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER)== GL_FRAMEBUFFER_COMPLETE;
		KITA_CORE_ASSERT(status , "frame buffer is not completed");

		UnBind();

	}

	

}