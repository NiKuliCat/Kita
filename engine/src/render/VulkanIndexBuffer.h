#pragma once

#include "VulkanBuffer.h"
namespace Kita {

	class VulkanContext;

	class VulkanIndexBuffer
	{
	public:
		struct CreateInfo
		{
			std::string Name;

			const uint32_t* InitialData = nullptr;
			uint32_t Count = 0;

			// true: HOST_VISIBLE, suitable for dynamic updates
			// false: DEVICE_LOCAL, uploaded through staging
			bool Dynamic = false;

			// only meaningful when Dynamic == true
			bool PersistentMap = false;
		};

	public:
		VulkanIndexBuffer() = default;
		VulkanIndexBuffer(VulkanContext& context, const CreateInfo& createInfo);
		~VulkanIndexBuffer() = default;

		VulkanIndexBuffer(const VulkanIndexBuffer&) = delete;
		VulkanIndexBuffer& operator=(const VulkanIndexBuffer&) = delete;

		VulkanIndexBuffer(VulkanIndexBuffer&& other) noexcept = default;
		VulkanIndexBuffer& operator=(VulkanIndexBuffer&& other) noexcept = default;

		void Init(VulkanContext& context, const CreateInfo& createInfo);

		void SetData(const uint32_t* data, uint32_t count, uint32_t firstIndex = 0);

		uint32_t GetCount() const { return m_Count; }
		uint32_t GetSize() const { return m_Size; }
		bool IsDynamic() const { return m_Dynamic; }

		VkIndexType GetIndexType() const { return VK_INDEX_TYPE_UINT32; }
		VkBuffer GetHandle() const { return m_Buffer.GetHandle(); }

		const VulkanBuffer& GetBuffer() const { return m_Buffer; }
		VulkanBuffer& GetBuffer() { return m_Buffer; }

	private:
		VulkanBuffer m_Buffer;
		uint32_t m_Count = 0;
		uint32_t m_Size = 0;
		bool m_Dynamic = false;
	};

}
