#include "kita_pch.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanContext.h"
#include "VulkanGeometry.h"
#include "VulkanShader.h"
#include "core/Log.h"
namespace Kita {

	namespace
	{
		void VKCheck(VkResult result, const char* message)
		{
			if (result != VK_SUCCESS)
			{
				KITA_CORE_ERROR("{0}, VkResult = {1}", message, static_cast<int32_t>(result));
				throw std::runtime_error(message);
			}
		}
	}

	VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanContext& context, const CreateInfo& createInfo)
		: m_Context(&context),
		m_Name(createInfo.Name)
	{
		Create(createInfo);
	}

	VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
	{
		Destroy();
	}

	VulkanGraphicsPipeline::VulkanGraphicsPipeline(VulkanGraphicsPipeline&& other) noexcept
		: m_Context(other.m_Context),
		m_Name(std::move(other.m_Name)),
		m_PipelineLayout(other.m_PipelineLayout),
		m_Pipeline(other.m_Pipeline)
	{
		other.m_Context = nullptr;
		other.m_PipelineLayout = VK_NULL_HANDLE;
		other.m_Pipeline = VK_NULL_HANDLE;
	}

	VulkanGraphicsPipeline& VulkanGraphicsPipeline::operator=(VulkanGraphicsPipeline&& other) noexcept
	{
		if (this == &other)
			return *this;

		Destroy();

		m_Context = other.m_Context;
		m_Name = std::move(other.m_Name);
		m_PipelineLayout = other.m_PipelineLayout;
		m_Pipeline = other.m_Pipeline;

		other.m_Context = nullptr;
		other.m_PipelineLayout = VK_NULL_HANDLE;
		other.m_Pipeline = VK_NULL_HANDLE;

		return *this;
	}

	void VulkanGraphicsPipeline::Destroy()
	{
		if (!m_Context)
			return;

		VkDevice device = m_Context->GetDevice();
		if (device == VK_NULL_HANDLE)
		{
			m_Context = nullptr;
			return;
		}

		if (m_Pipeline != VK_NULL_HANDLE)
		{
			vkDestroyPipeline(device, m_Pipeline, nullptr);
			m_Pipeline = VK_NULL_HANDLE;
		}

		if (m_PipelineLayout != VK_NULL_HANDLE)
		{
			vkDestroyPipelineLayout(device, m_PipelineLayout, nullptr);
			m_PipelineLayout = VK_NULL_HANDLE;
		}

		m_Name.clear();
		m_Context = nullptr;
	}

	void VulkanGraphicsPipeline::Bind(VkCommandBuffer commandBuffer) const
	{
		KITA_CORE_ASSERT(commandBuffer != VK_NULL_HANDLE, "VulkanGraphicsPipeline Bind commandBuffer is null");
		KITA_CORE_ASSERT(m_Pipeline != VK_NULL_HANDLE, "VulkanGraphicsPipeline handle is invalid");

		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_Pipeline);
	}

	void VulkanGraphicsPipeline::Create(const CreateInfo& createInfo)
	{
		KITA_CORE_ASSERT(m_Context, "VulkanGraphicsPipeline context is null");
		KITA_CORE_ASSERT(createInfo.VertexShader, "VulkanGraphicsPipeline vertex shader is null");
		KITA_CORE_ASSERT(createInfo.FragmentShader, "VulkanGraphicsPipeline fragment shader is null");
		KITA_CORE_ASSERT(createInfo.Geometry, "VulkanGraphicsPipeline geometry is null");
		KITA_CORE_ASSERT(createInfo.ColorFormat != VK_FORMAT_UNDEFINED, "VulkanGraphicsPipeline color format is invalid");

		std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages =
		{
			createInfo.VertexShader->GetStageCreateInfo(),
			createInfo.FragmentShader->GetStageCreateInfo()
		};

		VkVertexInputBindingDescription bindingDescription =
			createInfo.Geometry->GetBindingDescription(0);
		std::vector<VkVertexInputAttributeDescription> attributeDescriptions =
			createInfo.Geometry->GetAttributeDescriptions(0, 0);

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = createInfo.Topology;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = nullptr;
		viewportState.scissorCount = 1;
		viewportState.pScissors = nullptr;

		std::array<VkDynamicState, 2> dynamicStates =
		{
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = createInfo.PolygonMode;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = createInfo.CullMode;
		rasterizer.frontFace = createInfo.FrontFace;
		rasterizer.depthBiasEnable = VK_FALSE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = createInfo.EnableDepthTest ? VK_TRUE : VK_FALSE;
		depthStencil.depthWriteEnable = createInfo.EnableDepthWrite ? VK_TRUE : VK_FALSE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		depthStencil.depthBoundsTestEnable = VK_FALSE;
		depthStencil.stencilTestEnable = VK_FALSE;

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask =
			VK_COLOR_COMPONENT_R_BIT |
			VK_COLOR_COMPONENT_G_BIT |
			VK_COLOR_COMPONENT_B_BIT |
			VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = createInfo.EnableBlending ? VK_TRUE : VK_FALSE;

		if (createInfo.EnableBlending)
		{
			colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
			colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
		}

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;



		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(createInfo.DescriptorSetLayouts.size());
		pipelineLayoutInfo.pSetLayouts = createInfo.DescriptorSetLayouts.empty() ? nullptr : createInfo.DescriptorSetLayouts.data();

		VkPushConstantRange pushConstantRange{};
		if (createInfo.PushConstantSize > 0)
		{
			pushConstantRange.stageFlags = createInfo.PushConstantStages;
			pushConstantRange.offset = 0;
			pushConstantRange.size = createInfo.PushConstantSize;

			pipelineLayoutInfo.pushConstantRangeCount = 1;
			pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
		}
		else
		{
			pipelineLayoutInfo.pushConstantRangeCount = 0;
			pipelineLayoutInfo.pPushConstantRanges = nullptr;
		}

		VKCheck(
			vkCreatePipelineLayout(m_Context->GetDevice(), &pipelineLayoutInfo, nullptr, &m_PipelineLayout),
			"Failed to create Vulkan pipeline layout");

		VkPipelineRenderingCreateInfo renderingInfo{};
		renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
		renderingInfo.colorAttachmentCount = 1;
		renderingInfo.pColorAttachmentFormats = &createInfo.ColorFormat;
		renderingInfo.depthAttachmentFormat = createInfo.DepthFormat;
		renderingInfo.stencilAttachmentFormat = VK_FORMAT_UNDEFINED;

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.pNext = &renderingInfo;
		pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
		pipelineInfo.pStages = shaderStages.data();
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.layout = m_PipelineLayout;
		pipelineInfo.renderPass = VK_NULL_HANDLE;
		pipelineInfo.subpass = 0;
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

		VKCheck(
			vkCreateGraphicsPipelines(m_Context->GetDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_Pipeline),
			"Failed to create Vulkan graphics pipeline");

		KITA_CORE_INFO("Created VulkanGraphicsPipeline '{0}'", m_Name);
	}

}
