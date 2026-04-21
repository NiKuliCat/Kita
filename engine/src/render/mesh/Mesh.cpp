#include "kita_pch.h"

#include "Mesh.h"
#include "core/Log.h"
namespace Kita
{
    Mesh::Mesh(const std::vector<Vertex>& vertex, const std::vector<uint32_t> indices)
		:m_Vertices(vertex), m_Indices(indices)
    {
        InitBuffer();
    }
    Mesh::~Mesh()
    {
    }
    void Mesh::InitBuffer()
    {
        BufferLayout boxLayout = {
           {ShaderDataType::Float3,"position"},
           {ShaderDataType::Float4,"color"},
           {ShaderDataType::Float2,"texcoords"},
           {ShaderDataType::Float3,"normal"},
           {ShaderDataType::Float3,"tangent"},
           {ShaderDataType::Float3,"bitangent"}
        };

        m_VBO = VertexBuffer::Create(m_Vertices.data(), static_cast<uint32_t>(sizeof(Vertex) * m_Vertices.size()));
        m_VBO->SetLayout(boxLayout);
        m_IBO = IndexBuffer::Create(m_Indices.data(), static_cast<uint32_t>(m_Indices.size()));

        m_VAO = VertexArray::Create();
        m_VAO->AddVertexBuffer(m_VBO);
        m_VAO->SetIndexBuffer(m_IBO);
    }
}