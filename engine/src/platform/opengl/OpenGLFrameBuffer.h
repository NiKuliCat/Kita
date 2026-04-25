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

		virtual const uint32_t GetID() const override { return m_FrameBufferID; }
		virtual uint32_t GetColorAttachment(uint32_t index = 0) const override;
		virtual uint32_t GetDepthAttachment() const override;

		virtual int GetIDBufferValue(int x, int y, uint32_t index) const override;
		virtual void ClearIDBuffer(int value, uint32_t index) const override;

		virtual void ReSize(uint32_t width, uint32_t height) override;
		virtual glm::ivec2 GetSize()  override { return glm::ivec2(m_Descriptor.Width, m_Descriptor.Height); }

		virtual void Bind() override;
		virtual void UnBind() override;

		void Invalidate();

		virtual bool IsMultisampled() const override { return m_Descriptor.Samples > 1; }
		virtual void BlitColorTo(const Ref<FrameBuffer>& target, uint32_t srcAttachment = 0, uint32_t dstAttachment = 0) const override;

	private:
	


	private:
		FrameBufferDescriptor m_Descriptor;
	
		std::vector<FrameBufferTexDescription> m_ColorAttachmentsDesc;
		FrameBufferTexDescription m_DepthAttachmentDesc;



		uint32_t  m_FrameBufferID = 0;
		std::vector<uint32_t> m_ColorAttachments;
		uint32_t m_DepthAttachment = 0;
	};
}