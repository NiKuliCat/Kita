#include "kita_pch.h"
#include "UniformBuffer.h"
#include "core/Log.h"
namespace Kita {
	Ref<UniformBuffer> UniformBuffer::Create(uint32_t size, uint32_t binding)
	{
		(void)size;
		(void)binding;
		throw std::runtime_error("Legacy UniformBuffer::Create path is disabled during Vulkan-only migration.");
		return nullptr;
	}
}
