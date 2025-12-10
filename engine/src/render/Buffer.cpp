#include "kita_pch.h"
#include "core/Log.h"
#include "Buffer.h"
#include "RendererAPI.h"
#include "platform/opengl/OpenGLBuffer.h"
namespace Kita {


	Ref<VertexBuffer> VertexBuffer::Create(uint32_t size)
	{
		switch (RendererAPI::GetAPI())
		{
			case RendererAPI::API::OpenGL:
			{
				return CreateRef<OpenGLVertexBuffer>(size);
				break;
			}
			case RendererAPI::API::None:
			{
				return nullptr;
				break;
			}
		}

		KITA_CORE_ERROR("UnKown RendererAPI !");
		return nullptr;
	}

	Ref<VertexBuffer> VertexBuffer::Create(void* vertices, uint32_t size)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
		{
			return CreateRef<OpenGLVertexBuffer>(vertices,size);
			break;
		}
		case RendererAPI::API::None:
		{
			return nullptr;
			break;
		}
		}

		KITA_CORE_ERROR("UnKown RendererAPI !");
		return nullptr;
	}

	Ref<IndexBuffer> IndexBuffer::Create(uint32_t* indices, uint32_t count)
	{
		switch (RendererAPI::GetAPI())
		{
		case RendererAPI::API::OpenGL:
		{
			return CreateRef<OpenGLIndexBuffer>(indices, count);
			break;
		}
		case RendererAPI::API::None:
		{
			return nullptr;
			break;
		}
		}

		KITA_CORE_ERROR("UnKown RendererAPI !");
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