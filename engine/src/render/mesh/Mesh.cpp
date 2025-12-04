#include "kita_pch.h"

#include "Mesh.h"
#include "core/Log.h"
namespace Kita
{
    Mesh::Mesh(const std::vector<Vertex>& vertex, const std::vector<uint32_t> indices)
		:m_Vertices(vertex), m_Indices(indices)
    {
    }
    Mesh::~Mesh()
    {
    }
}