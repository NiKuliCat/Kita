#pragma once
#include "render/VertexArray.h"


namespace Kita {

	class OpenGLVertexArray : public VertexArray
	{
	public:
		OpenGLVertexArray();

		virtual ~OpenGLVertexArray();

		virtual void Bind()const override;
		virtual void UnBind()const override;

		virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) override;
		virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) override;

		virtual const  Ref<IndexBuffer>& GetIndexBuffer() const override { return m_IndexBuffer; }
		virtual const uint32_t GetIndexCount() const override { return m_IndexBuffer->GetCount(); }

		virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const override { return m_VertexBuffers; }
	private:

		uint32_t m_VertexArrayID;

		std::vector<Ref<VertexBuffer>> m_VertexBuffers;
		Ref<IndexBuffer> m_IndexBuffer;
	};
}