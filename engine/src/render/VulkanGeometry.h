#pragma once

#include "VulkanBuffer.h"
#include "render/BufferLayout.h"
#include <vulkan/vulkan.h>
namespace Kita {

	class VulkanContext;

	class VulkanGeometry
	{
	public:
		struct VertexStream
		{
			uint32_t Binding = 0;
			BufferLayout Layout{};

			VulkanBuffer Buffer;
			VkDeviceSize Offset = 0;
		};

		struct CreateInfo
		{
			std::string Name;

			const void* VertexData = nullptr;
			uint32_t VertexDataSize = 0;
			uint32_t VertexCount = 0;
			BufferLayout VertexLayout{};

			const uint32_t* IndexData = nullptr;
			uint32_t IndexCount = 0;

			bool Dynamic = false;
			bool PersistentMap = false;
		};

	public:
		VulkanGeometry() = default;
		VulkanGeometry(VulkanContext& context, const CreateInfo& createInfo);
		~VulkanGeometry() = default;

		VulkanGeometry(const VulkanGeometry&) = delete;
		VulkanGeometry& operator=(const VulkanGeometry&) = delete;

		VulkanGeometry(VulkanGeometry&& other) noexcept = default;
		VulkanGeometry& operator=(VulkanGeometry&& other) noexcept = default;

		void Init(VulkanContext& context, const CreateInfo& createInfo);

		void SetVertexData(const void* data, uint32_t size, uint32_t offset = 0);
		void SetIndexData(const uint32_t* data, uint32_t count, uint32_t firstIndex = 0);

		bool HasIndices() const { return m_IndexBuffer.IsValid(); }
		bool IsDynamic() const { return m_Dynamic; }

		uint32_t GetVertexCount() const { return m_VertexCount; }
		uint32_t GetIndexCount() const { return m_IndexCount; }

		const BufferLayout& GetVertexLayout() const { return m_VertexLayout; }

		VkBuffer GetVertexBuffer() const { return m_VertexStream.Buffer.GetHandle(); }
		VkDeviceSize GetVertexBufferOffset() const { return m_VertexStream.Offset; }

		VkBuffer GetIndexBuffer() const { return m_IndexBuffer.GetHandle(); }
		VkDeviceSize GetIndexBufferOffset() const { return m_IndexBufferOffset; }
		VkIndexType GetIndexType() const { return VK_INDEX_TYPE_UINT32; }

		const VertexStream& GetVertexStream() const { return m_VertexStream; }
		const VulkanBuffer& GetVertexBufferResource() const { return m_VertexStream.Buffer; }
		const VulkanBuffer& GetIndexBufferResource() const { return m_IndexBuffer; }


		VkVertexInputBindingDescription GetBindingDescription(uint32_t binding = 0) const;
		std::vector<VkVertexInputAttributeDescription> GetAttributeDescriptions(uint32_t binding = 0, uint32_t firstLocation = 0) const;
	private:
		std::string m_Name;
		bool m_Dynamic = false;

		VertexStream m_VertexStream{};
		uint32_t m_VertexCount = 0;
		uint32_t m_VertexDataSize = 0;

		VulkanBuffer m_IndexBuffer;
		VkDeviceSize m_IndexBufferOffset = 0;
		uint32_t m_IndexCount = 0;

		BufferLayout m_VertexLayout{};
	};

}
