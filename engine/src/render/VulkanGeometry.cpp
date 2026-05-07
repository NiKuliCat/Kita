#include "kita_pch.h"
#include "VulkanGeometry.h"
#include "VulkanContext.h"

#include "core/Log.h"
namespace Kita {

	namespace
	{
		VkFormat ShaderDataTypeToVulkanFormat(Kita::ShaderDataType type)
		{
			switch (type)
			{
			case Kita::ShaderDataType::Float:  return VK_FORMAT_R32_SFLOAT;
			case Kita::ShaderDataType::Float2: return VK_FORMAT_R32G32_SFLOAT;
			case Kita::ShaderDataType::Float3: return VK_FORMAT_R32G32B32_SFLOAT;
			case Kita::ShaderDataType::Float4: return VK_FORMAT_R32G32B32A32_SFLOAT;

			case Kita::ShaderDataType::Int:  return VK_FORMAT_R32_SINT;
			case Kita::ShaderDataType::Int2: return VK_FORMAT_R32G32_SINT;
			case Kita::ShaderDataType::Int3: return VK_FORMAT_R32G32B32_SINT;
			case Kita::ShaderDataType::Int4: return VK_FORMAT_R32G32B32A32_SINT;

			case Kita::ShaderDataType::Bool: return VK_FORMAT_R8_UINT;

			case Kita::ShaderDataType::Mat3:
			case Kita::ShaderDataType::Mat4:
			case Kita::ShaderDataType::None:
			default:
				KITA_CORE_ASSERT(false, "Unsupported ShaderDataType for Vulkan vertex input");
				return VK_FORMAT_UNDEFINED;
			}
		}
	}

	VulkanGeometry::VulkanGeometry(VulkanContext& context, const CreateInfo& createInfo)
	{
		Init(context, createInfo);
	}

	void VulkanGeometry::Init(VulkanContext& context, const CreateInfo& createInfo)
	{
		KITA_CORE_ASSERT(createInfo.VertexDataSize > 0, "VulkanGeometry vertex data size must be greater than zero");
		KITA_CORE_ASSERT(createInfo.VertexCount > 0, "VulkanGeometry vertex count must be greater than zero");
		KITA_CORE_ASSERT(
			createInfo.VertexLayout.GetElements().size() > 0,
			"VulkanGeometry vertex layout must not be empty");

		m_Name = createInfo.Name;
		m_Dynamic = createInfo.Dynamic;
		m_VertexCount = createInfo.VertexCount;
		m_VertexDataSize = createInfo.VertexDataSize;
		m_IndexCount = createInfo.IndexCount;
		m_VertexLayout = createInfo.VertexLayout;

		m_VertexStream.Binding = 0;
		m_VertexStream.Layout = createInfo.VertexLayout;
		m_VertexStream.Offset = 0;

		VulkanBuffer::CreateInfo vertexBufferInfo{};
		vertexBufferInfo.Name = createInfo.Name.empty() ? "GeometryVertexBuffer" : (createInfo.Name + "_VB");
		vertexBufferInfo.Size = createInfo.VertexDataSize;
		vertexBufferInfo.InitialData = createInfo.VertexData;
		vertexBufferInfo.InitialDataSize = createInfo.VertexData ? createInfo.VertexDataSize : 0;

		if (createInfo.Dynamic)
		{
			vertexBufferInfo.Usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
			vertexBufferInfo.MemoryProperties =
				VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			vertexBufferInfo.PersistentMap = createInfo.PersistentMap;
		}
		else
		{
			vertexBufferInfo.Usage =
				VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
				VK_BUFFER_USAGE_TRANSFER_DST_BIT;
			vertexBufferInfo.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
			vertexBufferInfo.PersistentMap = false;
		}

		m_VertexStream.Buffer.Init(context, vertexBufferInfo);

		if (createInfo.IndexCount > 0)
		{
			VulkanBuffer::CreateInfo indexBufferInfo{};
			indexBufferInfo.Name = createInfo.Name.empty() ? "GeometryIndexBuffer" : (createInfo.Name + "_IB");
			indexBufferInfo.Size = static_cast<VkDeviceSize>(createInfo.IndexCount) * sizeof(uint32_t);
			indexBufferInfo.InitialData = createInfo.IndexData;
			indexBufferInfo.InitialDataSize = createInfo.IndexData
				? static_cast<VkDeviceSize>(createInfo.IndexCount) * sizeof(uint32_t)
				: 0;

			if (createInfo.Dynamic)
			{
				indexBufferInfo.Usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
				indexBufferInfo.MemoryProperties =
					VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
					VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
				indexBufferInfo.PersistentMap = createInfo.PersistentMap;
			}
			else
			{
				indexBufferInfo.Usage =
					VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
					VK_BUFFER_USAGE_TRANSFER_DST_BIT;
				indexBufferInfo.MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
				indexBufferInfo.PersistentMap = false;
			}

			m_IndexBuffer.Init(context, indexBufferInfo);
			m_IndexBufferOffset = 0;
		}
	}

	void VulkanGeometry::SetVertexData(const void* data, uint32_t size, uint32_t offset)
	{
		KITA_CORE_ASSERT(data, "VulkanGeometry SetVertexData data is null");
		KITA_CORE_ASSERT(size > 0, "VulkanGeometry SetVertexData size must be greater than zero");
		KITA_CORE_ASSERT(offset + size <= m_VertexDataSize, "VulkanGeometry vertex data range is out of bounds");

		m_VertexStream.Buffer.SetData(data, size, offset);
	}

	void VulkanGeometry::SetIndexData(const uint32_t* data, uint32_t count, uint32_t firstIndex)
	{
		KITA_CORE_ASSERT(HasIndices(), "VulkanGeometry has no index buffer");
		KITA_CORE_ASSERT(data, "VulkanGeometry SetIndexData data is null");
		KITA_CORE_ASSERT(count > 0, "VulkanGeometry SetIndexData count must be greater than zero");
		KITA_CORE_ASSERT(firstIndex + count <= m_IndexCount, "VulkanGeometry index data range is out of bounds");

		const uint32_t sizeInBytes = count * static_cast<uint32_t>(sizeof(uint32_t));
		const uint32_t offsetInBytes = firstIndex * static_cast<uint32_t>(sizeof(uint32_t));

		m_IndexBuffer.SetData(data, sizeInBytes, offsetInBytes);
	}

	VkVertexInputBindingDescription VulkanGeometry::GetBindingDescription(uint32_t binding) const
	{
		KITA_CORE_ASSERT(
			m_VertexLayout.GetElements().size() > 0,
			"VulkanGeometry vertex layout is empty");

		VkVertexInputBindingDescription bindingDescription{};
		bindingDescription.binding = binding;
		bindingDescription.stride = m_VertexLayout.GetVertexStride();
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescription;
	}

	std::vector<VkVertexInputAttributeDescription> VulkanGeometry::GetAttributeDescriptions(
		uint32_t binding,
		uint32_t firstLocation) const
	{
		KITA_CORE_ASSERT(
			m_VertexLayout.GetElements().size() > 0,
			"VulkanGeometry vertex layout is empty");

		std::vector<VkVertexInputAttributeDescription> attributes;
		attributes.reserve(m_VertexLayout.GetElements().size());

		uint32_t location = firstLocation;

		for (const auto& element : m_VertexLayout)
		{
			KITA_CORE_ASSERT(
				element.DataType != ShaderDataType::Mat3 && element.DataType != ShaderDataType::Mat4,
				"Mat3/Mat4 vertex attributes are not supported in this helper yet");

			VkVertexInputAttributeDescription attribute{};
			attribute.location = location++;
			attribute.binding = binding;
			attribute.format = ShaderDataTypeToVulkanFormat(element.DataType);
			attribute.offset = element.Offset;

			attributes.push_back(attribute);
		}

		return attributes;
	}

}