#pragma once

#include "Buffer.h"
namespace Kita {

	class VertexArray
	{
	public:
		virtual ~VertexArray(){}

		virtual void Bind() const = 0;
		virtual void UnBind() const = 0;


		virtual void AddVertexBuffer(const Ref<VertexBuffer>& vertexBuffer) = 0;
		virtual void SetIndexBuffer(const Ref<IndexBuffer>& indexBuffer) = 0;


		static Ref<VertexArray> Create();
	};
}