#include "kita_pch.h"
#include "core/Log.h"
#include "Buffer.h"
namespace Kita {


	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		(void)size;
		throw std::runtime_error("Legacy VertexBuffer::Create path is disabled during Vulkan-only migration.");
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(void* vertices, uint32_t size)
	{
		(void)vertices;
		(void)size;
		throw std::runtime_error("Legacy VertexBuffer::Create(data) path is disabled during Vulkan-only migration.");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
	{
		(void)indices;
		(void)count;
		throw std::runtime_error("Legacy IndexBuffer::Create path is disabled during Vulkan-only migration.");
		return nullptr;
	}

	uint32_t BufferLayout::CaculateVertexStrideAndOffset()
	{
		uint32_t offset = 0;
		m_Stride = 0;
		for (auto& element : m_Elements)
		{
			element.Offset = offset;
			offset += element.Size;
			m_Stride += element.Size;
		}


		return m_Stride;
	}

}
