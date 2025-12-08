#pragma once

#include "core/Core.h"



namespace Kita {


	struct FrameBufferDescriptor
	{
		uint32_t Width, Height;
		uint32_t Samples = 1;

		bool SwapChainTarget = false;
	};

	class FrameBuffer 
	{

	public:
		virtual ~FrameBuffer() = default;
		virtual  FrameBufferDescriptor& GetDescriptor() = 0;
		virtual const FrameBufferDescriptor& GetDescriptor() const = 0;

		virtual uint32_t GetColorAttachment(uint32_t index = 0) const = 0;
		virtual uint32_t GetDepthAttachment() const = 0;

		virtual void ReSize(uint32_t width, uint32_t height) = 0;

		virtual void Bind() = 0;
		virtual void UnBind() = 0;

		static Ref<FrameBuffer> Create(const FrameBufferDescriptor& descriptor);
	 };
}