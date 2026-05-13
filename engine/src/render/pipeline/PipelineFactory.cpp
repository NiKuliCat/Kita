#include "kita_pch.h"

#include "PipelineFactory.h"
#include "render/VulkanContext.h"
#include "render/VulkanGeometry.h"
#include "render/VulkanShader.h"
#include "core/Log.h"
namespace Kita {


    namespace
    {
        template<typename T>
        void HashCombine(size_t& seed, const T& value)
        {
            std::hash<T> hasher;
            seed ^= hasher(value) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        }

        void ValidateRequest(const PipelineRequest& request)
        {
            KITA_CORE_ASSERT(request.Pass != PassType::Unknown, "PipelineRequest Pass is Unknown");
            KITA_CORE_ASSERT(request.Geometry, "PipelineRequest Geometry is null");
            KITA_CORE_ASSERT(request.VertexShader, "PipelineRequest VertexShader is null");
            KITA_CORE_ASSERT(request.FragmentShader, "PipelineRequest FragmentShader is null");
            KITA_CORE_ASSERT(request.ColorFormat != VK_FORMAT_UNDEFINED, "PipelineRequest ColorFormat is invalid");
            KITA_CORE_ASSERT(!request.DescriptorSetLayouts.empty(), "PipelineRequest DescriptorSetLayouts is empty");
        }

        const char* PassTypeToString(PassType pass)
        {
            switch (pass)
            {
            case PassType::ForwardOpaque:      return "ForwardOpaque";
            case PassType::ForwardTransparent: return "ForwardTransparent";
            case PassType::PostProcess:        return "PostProcess";
            case PassType::GBuffer:            return "GBuffer";
            case PassType::DeferredLighting:   return "DeferredLighting";
            case PassType::ShadowCaster:       return "ShadowCaster";
            case PassType::UI:                 return "UI";
            default:                           return "Unknown";
            }
        }
    }


	PipelineFactory::PipelineFactory(VulkanContext& context)
        :m_Context(&context)
	{
        KITA_CORE_ASSERT(m_Context, "VulkanPipelineFactory context is null");
	}

	PipelineFactory::~PipelineFactory()
	{
        Clear();
	}

	VulkanGraphicsPipeline* PipelineFactory::GetOrCreate(const PipelineRequest& request)
	{
        KITA_CORE_ASSERT(m_Context, "VulkanPipelineFactory context is null");
        ValidateRequest(request);

        PipelineKey key = BuildKey(request);

        auto it = m_Pipelines.find(key);
        if (it != m_Pipelines.end())
            return it->second.get();

        Unique<VulkanGraphicsPipeline> pipeline = CreatePipeline(request, key);
        KITA_CORE_ASSERT(pipeline, "Failed to create VulkanGraphicsPipeline");

        VulkanGraphicsPipeline* raw = pipeline.get();
        m_Pipelines.emplace(std::move(key), std::move(pipeline));
        return raw;
	}

	void PipelineFactory::Clear()
	{
        m_Pipelines.clear();
	}

	PipelineKey PipelineFactory::BuildKey(const PipelineRequest& request) const
	{
        PipelineKey key{};
        key.Pass = request.Pass;

        key.ColorFormat = request.ColorFormat;
        key.DepthFormat = request.DepthFormat;
        key.Samples = request.Samples;

        key.Topology = request.Topology;
        key.PolygonMode = request.PolygonMode;
        key.CullMode = request.CullMode;
        key.FrontFace = request.FrontFace;

        key.EnableDepthTest = request.EnableDepthTest;
        key.EnableDepthWrite = request.EnableDepthWrite;
        key.EnableBlending = request.EnableBlending;

        key.VertexModule = request.VertexShader ? request.VertexShader->GetShaderModule() : VK_NULL_HANDLE;
        key.FragmentModule = request.FragmentShader ? request.FragmentShader->GetShaderModule() : VK_NULL_HANDLE;

        key.VertexLayoutHash = BuildVertexLayoutHash(*request.Geometry);

        key.PushConstantStages = request.PushConstantStages;
        key.PushConstantSize = request.PushConstantSize;

        return key;
	}

	uint64_t PipelineFactory::BuildVertexLayoutHash(const VulkanGeometry& geometry) const
	{
        const BufferLayout& layout = geometry.GetVertexLayout();

        size_t seed = 0;
        HashCombine(seed, layout.GetVertexStride());

        for (const BufferElement& element : layout)
        {
            HashCombine(seed, static_cast<uint32_t>(element.DataType));
            HashCombine(seed, element.Size);
            HashCombine(seed, element.Count);
            HashCombine(seed, element.Offset);
            HashCombine(seed, element.Normalized);
            HashCombine(seed, element.Name);
        }

        return static_cast<uint64_t>(seed);
	}

	Unique<VulkanGraphicsPipeline> PipelineFactory::CreatePipeline(const PipelineRequest& request, const PipelineKey& key) const
	{
        KITA_CORE_ASSERT(m_Context, "VulkanPipelineFactory context is null");

        VulkanGraphicsPipeline::CreateInfo createInfo{};
        createInfo.Name =
            std::string(PassTypeToString(request.Pass)) +
            "_VS_" + request.VertexShader->GetName() +
            "_FS_" + request.FragmentShader->GetName() +
            "_VL_" + std::to_string(key.VertexLayoutHash);

        createInfo.VertexShader = request.VertexShader;
        createInfo.FragmentShader = request.FragmentShader;
        createInfo.Geometry = request.Geometry;

        createInfo.PushConstantStages = request.PushConstantStages;
        createInfo.PushConstantSize = request.PushConstantSize;

        createInfo.ColorFormat = request.ColorFormat;
        createInfo.DepthFormat = request.DepthFormat;

        createInfo.Topology = request.Topology;
        createInfo.PolygonMode = request.PolygonMode;
        createInfo.CullMode = request.CullMode;
        createInfo.FrontFace = request.FrontFace;

        createInfo.EnableDepthTest = request.EnableDepthTest;
        createInfo.EnableDepthWrite = request.EnableDepthWrite;
        createInfo.EnableBlending = request.EnableBlending;

        createInfo.DescriptorSetLayouts = request.DescriptorSetLayouts;

        return CreateUnique<VulkanGraphicsPipeline>(*m_Context, createInfo);
	}

	size_t PipelineKeyHasher::operator()(const PipelineKey& key) const
	{
        size_t seed = 0;

        HashCombine(seed, static_cast<uint32_t>(key.Pass));
        HashCombine(seed, static_cast<uint32_t>(key.ColorFormat));
        HashCombine(seed, static_cast<uint32_t>(key.DepthFormat));
        HashCombine(seed, static_cast<uint32_t>(key.Samples));

        HashCombine(seed, static_cast<uint32_t>(key.Topology));
        HashCombine(seed, static_cast<uint32_t>(key.PolygonMode));
        HashCombine(seed, static_cast<uint32_t>(key.CullMode));
        HashCombine(seed, static_cast<uint32_t>(key.FrontFace));

        HashCombine(seed, key.EnableDepthTest);
        HashCombine(seed, key.EnableDepthWrite);
        HashCombine(seed, key.EnableBlending);

        HashCombine(seed, reinterpret_cast<uint64_t>(key.VertexModule));
        HashCombine(seed, reinterpret_cast<uint64_t>(key.FragmentModule));

        HashCombine(seed, key.VertexLayoutHash);
        HashCombine(seed, static_cast<uint32_t>(key.PushConstantStages));
        HashCombine(seed, key.PushConstantSize);

        return seed;
	}

}