#include "kita_pch.h"
#include "OpenGLVertexArray.h"
#include "core/Log.h"
#include <glad/glad.h>
namespace Kita {

	static GLenum ShaderDataTypeToOpenGL(ShaderDataType type)
	{
		switch (type)
		{
			case ShaderDataType::Float:		return GL_FLOAT;
			case ShaderDataType::Float2:	return GL_FLOAT;
			case ShaderDataType::Float3:	return GL_FLOAT;
			case ShaderDataType::Float4:	return GL_FLOAT;
			case ShaderDataType::Int:		return GL_INT;
			case ShaderDataType::Int2:		return GL_INT;
			case ShaderDataType::Int3:		return GL_INT;
			case ShaderDataType::Int4:		return GL_INT;
			case ShaderDataType::Mat3:		return GL_FLOAT;
			case ShaderDataType::Mat4:		return GL_FLOAT;
			case ShaderDataType::Bool:		return GL_BOOL;
		}	 

		KITA_CORE_ASSERT(false, "Unkown ShaderDataType!");
		return 0;
	}

	OpenGLVertexArray::OpenGLVertexArray()
	{
		glCreateVertexArrays(1, &m_VertexArrayID);
	}
	OpenGLVertexArray::~OpenGLVertexArray()
	{
		glDeleteVertexArrays(1, &m_VertexArrayID);
	}
	void OpenGLVertexArray::Bind() const
	{
		glBindVertexArray(m_VertexArrayID);
	}
	void OpenGLVertexArray::UnBind() const
	{
		glBindVertexArray(0);
	}
	void OpenGLVertexArray::AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer)
	{
		
		KITA_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "vertex buffer not set layout");

		Bind();
		vertexBuffer->Bind();
		uint32_t index = 0;

		const auto& layout = vertexBuffer->GetLayout();

		for (auto& element : layout)
		{
			glEnableVertexAttribArray(index); // 启用顶点属性的位置索引， ==》 shader中的 layout (location = 0，1，2 ...)
			if (ShaderDataTypeToOpenGL(element.DataType) != GL_INT)
			{
				glVertexAttribPointer(
					index, element.Count,
					ShaderDataTypeToOpenGL(element.DataType),
					element.Normalized ? GL_TRUE : GL_FALSE,
					layout.GetVertexStride(), (void*)element.Offset);
			}
			else
			{
				glVertexAttribIPointer(
					index, element.Count,
					ShaderDataTypeToOpenGL(element.DataType),
					layout.GetVertexStride(), (void*)element.Offset);
			}
			index++;
		}
		m_VertexBuffers.push_back(vertexBuffer);

		UnBind();

	}
	void OpenGLVertexArray::SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer)
	{
		Bind();

		indexBuffer->Bind();
		m_IndexBuffer = indexBuffer;

		UnBind();
	}
}