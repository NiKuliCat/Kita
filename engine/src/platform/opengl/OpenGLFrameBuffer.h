#pragma once
#include "render/FrameBuffer.h"

#include <glad/glad.h>


namespace Kita {

	class OpenGLFrameBuffer : public FrameBuffer
	{
	public:
		OpenGLFrameBuffer(const FrameBufferDescriptor& descriptor);
		virtual ~OpenGLFrameBuffer();

		virtual FrameBufferDescriptor& GetDescriptor() override { return m_Descriptor; }
		virtual const FrameBufferDescriptor& GetDescriptor() const override { return m_Descriptor; }


		virtual uint32_t GetColorAttachment(uint32_t index = 0) const override;
		virtual uint32_t GetDepthAttachment() const override;

		virtual void ReSize(uint32_t width, uint32_t height) override;

		virtual void Bind() override;
		virtual void UnBind() override;

		void Invalidate();


	private:
	


	private:
		FrameBufferDescriptor m_Descriptor;
		uint32_t  m_FrameBufferID;
		uint32_t m_ColorAttachment;
		uint32_t m_DepthAttachment;
	};
}