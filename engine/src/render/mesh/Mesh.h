#pragma once
#include <filesystem>
#include <glm/glm.hpp>
#include "core/Core.h"
#include "render/BufferLayout.h"
namespace Kita {

	class VulkanContext;
	class VulkanGeometry;

	struct Vertex
	{
		glm::vec3 position;
		glm::vec4 color;
		glm::vec2 texcoords;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 bitangent;
	};

	static_assert(std::is_standard_layout_v<Vertex>, "Mesh::Vertex must remain standard-layout");



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

		void CreateVulkanGeometry(VulkanContext& context);
		Ref<VulkanGeometry>& GetVulkanGeometry() { return m_VulkanGeometry; }
		const Ref<VulkanGeometry>& GetVulkanGeometry() const { return m_VulkanGeometry; }

	private:

	private:
		std::vector<Vertex> m_Vertices;
		std::vector<uint32_t> m_Indices;
		Ref<VulkanGeometry> m_VulkanGeometry = nullptr;
	};
}
