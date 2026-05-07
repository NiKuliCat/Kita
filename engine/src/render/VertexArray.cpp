#include "kita_pch.h"
#include "VertexArray.h"
#include "core/Log.h"
namespace Kita {

	Ref<VertexArray> VertexArray::Create()
	{
		throw std::runtime_error("Legacy VertexArray::Create path is disabled during Vulkan-only migration.");
		return nullptr;
	}

}
