#pragma once

#include "core/Core.h"
#include "render/VulkanGraphicsPipeline.h"
#include "render/pass/RenderDataStruct.h"

namespace Kita {

	class VulkanContext;
	class VulkanGeometry;
	class VulkanShader;

	struct MaterialPipelineDesc
	{
        const VulkanShader* VertexShader = nullptr;
        const VulkanShader* FragmentShader = nullptr;

        VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPolygonMode PolygonMode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags CullMode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        bool EnableDepthTest = true;
        bool EnableDepthWrite = true;
        bool EnableBlending = false;

        VkShaderStageFlags PushConstantStages =
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
        uint32_t PushConstantSize = ObjectDataSize;
	};

    struct PipelineRequest
    {
        PassType Pass = PassType::Unknown;

        const VulkanGeometry* Geometry = nullptr;
        bool UseVertexInput = true;
        const VulkanShader* VertexShader = nullptr;
        const VulkanShader* FragmentShader = nullptr;

        VkFormat ColorFormat = VK_FORMAT_UNDEFINED;
        VkFormat DepthFormat = VK_FORMAT_UNDEFINED;
        VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;

        std::vector<VkDescriptorSetLayout> DescriptorSetLayouts;

        VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPolygonMode PolygonMode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags CullMode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        bool EnableDepthTest = true;
        bool EnableDepthWrite = true;
        bool EnableBlending = false;

        VkShaderStageFlags PushConstantStages = 0;
        uint32_t PushConstantSize = 0;
    };


    struct PipelineKey
    {
        PassType Pass = PassType::Unknown;

        VkFormat ColorFormat = VK_FORMAT_UNDEFINED;
        VkFormat DepthFormat = VK_FORMAT_UNDEFINED;
        VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;

        VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        VkPolygonMode PolygonMode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags CullMode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace FrontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

        bool EnableDepthTest = true;
        bool EnableDepthWrite = true;
        bool EnableBlending = false;

        VkShaderModule VertexModule = VK_NULL_HANDLE;
        VkShaderModule FragmentModule = VK_NULL_HANDLE;
        uint64_t DescriptorSetLayoutHash = 0;

        uint64_t VertexLayoutHash = 0;
        bool UseVertexInput = true;

        VkShaderStageFlags PushConstantStages = 0;
        uint32_t PushConstantSize = 0;

        bool operator==(const PipelineKey& other) const
        {
            return Pass == other.Pass &&
                ColorFormat == other.ColorFormat &&
                DepthFormat == other.DepthFormat &&
                Samples == other.Samples &&
                Topology == other.Topology &&
                PolygonMode == other.PolygonMode &&
                CullMode == other.CullMode &&
                FrontFace == other.FrontFace &&
                EnableDepthTest == other.EnableDepthTest &&
                EnableDepthWrite == other.EnableDepthWrite &&
                EnableBlending == other.EnableBlending &&
                VertexModule == other.VertexModule &&
                FragmentModule == other.FragmentModule &&
                DescriptorSetLayoutHash == other.DescriptorSetLayoutHash &&
                VertexLayoutHash == other.VertexLayoutHash &&
                UseVertexInput == other.UseVertexInput &&
                PushConstantStages == other.PushConstantStages &&
                PushConstantSize == other.PushConstantSize;
        }
    };

    struct PipelineKeyHasher
    {
        size_t operator()(const PipelineKey& key) const;
    };

	class PipelineFactory
	{
    public:
        explicit PipelineFactory(VulkanContext& context);
        ~PipelineFactory();

        PipelineFactory(const PipelineFactory&) = delete;
        PipelineFactory& operator=(const PipelineFactory&) = delete;

        VulkanGraphicsPipeline* GetOrCreate(const PipelineRequest& request);

        void Clear();
        void InvalidateAll() { Clear(); }

        size_t GetPipelineCount() const { return m_Pipelines.size(); }

    private:
        PipelineKey BuildKey(const PipelineRequest& request) const;
        uint64_t BuildVertexLayoutHash(const VulkanGeometry& geometry) const;
        Unique<VulkanGraphicsPipeline> CreatePipeline(
            const PipelineRequest& request,
            const PipelineKey& key) const;

    private:
        VulkanContext* m_Context = nullptr;

        // 第一阶段先只做引擎层 pipeline 对象复用。
        // 后续如果要接入 Vulkan 原生 VkPipelineCache，可以在这里增加：
        // VkPipelineCache m_VkPipelineCache = VK_NULL_HANDLE;
        std::unordered_map<PipelineKey, Unique<VulkanGraphicsPipeline>, PipelineKeyHasher> m_Pipelines;
    };

}
