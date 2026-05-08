#pragma once
#include "VulkanBuffer.h"
#include "render/BufferLayout.h"

namespace Kita {

	class VulkanContext;

	class VulkanVertexBuffer
	{
	public:
		struct CreateInfo
		{
			std::string Name;
			uint32_t Size = 0;
			BufferLayout Layout{};

			const void* InitialData = nullptr;
			uint32_t InitialDataSize = 0;
			bool Dynamic = false;
			bool PersistentMap = false;
		};

	public:
		VulkanVertexBuffer() = default;
		VulkanVertexBuffer(VulkanContext& context, const CreateInfo& createInfo);
		~VulkanVertexBuffer() = default;

		VulkanVertexBuffer(const VulkanVertexBuffer&) = delete;
		VulkanVertexBuffer& operator=(const VulkanVertexBuffer&) = delete;

		VulkanVertexBuffer(VulkanVertexBuffer&& other) noexcept = default;
		VulkanVertexBuffer& operator=(VulkanVertexBuffer&& other) noexcept = default;

		void Init(VulkanContext& context, const CreateInfo& createInfo);

		void SetData(const void* data, uint32_t size, uint32_t offset = 0);

		void SetLayout(const BufferLayout& layout) { m_Layout = layout; }

		const BufferLayout& GetLayout() const { return m_Layout; }
		uint32_t GetSize() const { return m_Size; }

		bool IsDynamic() const { return m_Dynamic; }

		VkBuffer GetHandle() const { return m_Buffer.GetHandle(); }
		const VulkanBuffer& GetBuffer() const { return m_Buffer; }
		VulkanBuffer& GetBuffer() { return m_Buffer; }

	private:
		VulkanBuffer m_Buffer;
		BufferLayout m_Layout;
		uint32_t m_Size = 0;
		bool m_Dynamic = false;
	};

}