#pragma once
#include <glm/glm.hpp>
#include "core/Core.h"
namespace Kita {

	struct Vertex
	{
		glm::vec3 position;
		glm::vec4 color;
		glm::vec2 texcoords;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 bitangent;
	};



	class Mesh
	{
	public:
		Mesh(const std::vector<Vertex>& vertex, const std::vector<uint32_t> indices);
		~Mesh();


		const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
		std::vector<Vertex>& GetVertices()  { return m_Vertices; }

		const std::vector<uint32_t>& GetIndices() const { return m_Indices; }
		std::vector<uint32_t>& GetIndices() { return m_Indices; }

		static Ref<Mesh> Create(const std::vector<Vertex>& vertex, const std::vector<uint32_t> indices) { return CreateRef<Mesh>(vertex, indices); }


	private:


	private:
		std::vector<Vertex> m_Vertices;
		std::vector<uint32_t> m_Indices;

	};
}
