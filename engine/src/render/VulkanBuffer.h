#pragma once
#include <vulkan/vulkan.h>
namespace Kita {
	class VulkanContext;



	class VulkanBuffer
	{
	public:
		struct CreateInfo
		{
			std::string Name;
			VkDeviceSize Size = 0;
			VkBufferUsageFlags Usage = 0;
			VkMemoryPropertyFlags MemoryProperties = 0;

			const void* InitialData = nullptr;
			VkDeviceSize InitialDataSize = 0;

			bool PersistentMap = false;
		};

	public:
		VulkanBuffer() = default;
		VulkanBuffer(VulkanContext& context, const CreateInfo& createInfo);
		~VulkanBuffer();

		VulkanBuffer(const VulkanBuffer&) = delete;
		VulkanBuffer& operator=(const VulkanBuffer&) = delete;

		VulkanBuffer(VulkanBuffer&& other) noexcept;
		VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;

		void Init(VulkanContext& context, const CreateInfo& createInfo);
		void Destroy();
		bool IsValid() const { return m_Buffer != VK_NULL_HANDLE; }

		void* Map(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0);
		void Unmap();

		void Flush(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;
		void Invalidate(VkDeviceSize size = VK_WHOLE_SIZE, VkDeviceSize offset = 0) const;

		void SetData(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
		void CopyFrom(const VulkanBuffer& src, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);

		bool IsHostVisible() const;
		bool IsHostCoherent() const;
		bool IsMapped() const { return m_MappedData != nullptr; }

		VulkanContext& GetContext() const;
		VkBuffer GetHandle() const { return m_Buffer; }
		VkDeviceMemory GetMemory() const { return m_Memory; }
		VkDeviceSize GetSize() const { return m_Size; }
		VkBufferUsageFlags GetUsage() const { return m_Usage; }
		VkMemoryPropertyFlags GetMemoryProperties() const { return m_MemoryProperties; }
		const std::string& GetName() const { return m_Name; }

		static uint32_t FindMemoryType(VulkanContext& context, uint32_t typeFilter, VkMemoryPropertyFlags properties);


	private:
		void CreateBuffer(const CreateInfo& createInfo);
		void UploadWithStaging(const void* data, VkDeviceSize size, VkDeviceSize offset);
		void* GetMappedPointer(VkDeviceSize offset) const;

	private:
		VulkanContext* m_Context = nullptr;
		std::string m_Name;

		VkBuffer m_Buffer = VK_NULL_HANDLE;
		VkDeviceMemory m_Memory = VK_NULL_HANDLE;

		VkDeviceSize m_Size = 0;
		VkBufferUsageFlags m_Usage = 0;
		VkMemoryPropertyFlags m_MemoryProperties = 0;

		void* m_MappedData = nullptr;
		VkDeviceSize m_MappedOffset = 0;
		VkDeviceSize m_MappedSize = 0;
		bool m_PersistentMap = false;
	};

}
