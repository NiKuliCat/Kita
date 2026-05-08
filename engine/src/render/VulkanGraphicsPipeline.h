#pragma once
#include <vulkan/vulkan.h>
namespace Kita {

	class VulkanContext;
	class VulkanShader;
	class VulkanGeometry;

	class VulkanGraphicsPipeline
	{
	public:
		struct CreateInfo
		{
			std::string Name;

			const VulkanShader* VertexShader = nullptr;
			const VulkanShader* FragmentShader = nullptr;
			const VulkanGeometry* Geometry = nullptr;

			VkExtent2D Extent{};

			VkFormat ColorFormat = VK_FORMAT_UNDEFINED;
			VkFormat DepthFormat = VK_FORMAT_UNDEFINED;

			VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			VkPolygonMode PolygonMode = VK_POLYGON_MODE_FILL;
			VkCullModeFlags CullMode = VK_CULL_MODE_BACK_BIT;
			VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

			bool EnableDepthTest = true;
			bool EnableDepthWrite = true;
			bool EnableBlending = false;

			std::vector<VkDescriptorSetLayout> DescriptorSetLayouts{};
		};

	public:
		VulkanGraphicsPipeline(VulkanContext& context, const CreateInfo& createInfo);
		~VulkanGraphicsPipeline();

		VulkanGraphicsPipeline(const VulkanGraphicsPipeline&) = delete;
		VulkanGraphicsPipeline& operator=(const VulkanGraphicsPipeline&) = delete;

		VulkanGraphicsPipeline(VulkanGraphicsPipeline&& other) noexcept;
		VulkanGraphicsPipeline& operator=(VulkanGraphicsPipeline&& other) noexcept;

		void Destroy();
		void Bind(VkCommandBuffer commandBuffer) const;

		bool IsValid() const { return m_Pipeline != VK_NULL_HANDLE; }

		VkPipeline GetHandle() const { return m_Pipeline; }
		VkPipelineLayout GetLayout() const { return m_PipelineLayout; }
		const std::string& GetName() const { return m_Name; }

	private:
		void Create(const CreateInfo& createInfo);

	private:
		VulkanContext* m_Context = nullptr;
		std::string m_Name;

		VkPipelineLayout m_PipelineLayout = VK_NULL_HANDLE;
		VkPipeline m_Pipeline = VK_NULL_HANDLE;
	};

}
