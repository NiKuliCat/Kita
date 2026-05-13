#include "kita_pch.h"
#include "BufferLayout.h"
namespace Kita {

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
