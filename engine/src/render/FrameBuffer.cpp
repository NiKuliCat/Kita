#include "kita_pch.h"
#include "FrameBuffer.h"
#include "core/Log.h"
namespace Kita {

	Ref<FrameBuffer> FrameBuffer::Create(const FrameBufferDescriptor& descriptor)
	{
		(void)descriptor;
		throw std::runtime_error("Legacy FrameBuffer::Create path is disabled during Vulkan-only migration.");
		return nullptr;
	}

}
