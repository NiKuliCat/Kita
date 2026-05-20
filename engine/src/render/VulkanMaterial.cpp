#include "kita_pch.h"
#include "VulkanMaterial.h"
#include "VulkanContext.h"
#include "VulkanTexture.h"
#include "core/Log.h"

namespace Kita {
    namespace
    {
        const Ref<VulkanTexture>& FirstValidTexture(
            std::initializer_list<const Ref<VulkanTexture>*> candidates)
        {
            for (const Ref<VulkanTexture>* candidate : candidates)
            {
                if (candidate && *candidate && (*candidate)->IsValid())
                    return *candidate;
            }

            static const Ref<VulkanTexture> s_NullTexture = nullptr;
            return s_NullTexture;
        }
    }

    VulkanMaterial::VulkanMaterial(const Ref<VulkanShader>& vertex, const Ref<VulkanShader>& frag, const Ref<VulkanTexture>& tex)
        : m_VertexShader(vertex)
        , m_FragmentShader(frag)
        , m_AlbedoTexture(tex)
    {
    }

    VulkanMaterial::~VulkanMaterial()
    {
        Destroy();
    }

    void VulkanMaterial::SetParams(const MaterialGpuParams& params)
    {
        m_Params = params;
        MarkDescriptorSetsDirty();
    }

    void VulkanMaterial::UpdateParamBuffer(uint32_t frameIndex)
    {
        if (frameIndex >= m_ParamUBOs.size() || !m_ParamUBOs[frameIndex].IsValid())
            return;

        m_ParamUBOs[frameIndex].SetData(&m_Params, sizeof(MaterialGpuParams));
    }

    void VulkanMaterial::ClearTextures()
    {
        m_AlbedoTexture = nullptr;
        m_NormalTexture = nullptr;
        m_MetallicRoughnessTexture = nullptr;
        m_AmbientOcclusionTexture = nullptr;
        m_EmissiveTexture = nullptr;
        m_OpacityTexture = nullptr;
        MarkDescriptorSetsDirty();
    }

    void VulkanMaterial::InitDescriptors(VulkanContext& context, uint32_t framesInFlight)
    {
        DestroyDescriptorResources();

        m_Context = &context;
        const uint32_t descriptorCount = std::max(1u, framesInFlight);
        m_ParamUBOs.resize(descriptorCount);
        m_DescriptorSets.resize(descriptorCount);
        m_DescriptorDirtyFlags.assign(descriptorCount, 1);

        VulkanDescriptorSet::CreateInfo descInfo{};
        descInfo.Name = "Material_Set";
        descInfo.Bindings = {
            { AlbedoBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
            { ParamsBinding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
            { NormalBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
            { MetallicRoughnessBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
            { AmbientOcclusionBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
            { EmissiveBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
            { OpacityBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        };

        for (uint32_t i = 0; i < descriptorCount; ++i)
        {
            m_ParamUBOs[i].Init(context, sizeof(MaterialGpuParams), "MaterialParamsUBO_" + std::to_string(i));
            VulkanDescriptorSet::CreateInfo perFrameDescInfo = descInfo;
            perFrameDescInfo.Name += "_" + std::to_string(i);
            m_DescriptorSets[i].Init(context, perFrameDescInfo);
        }

        UpdateDescriptorSets();
    }

    void VulkanMaterial::UpdateDescriptorSets()
    {
        if (!m_Context)
            return;

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_DescriptorSets.size()); ++i)
        {
            UpdateDescriptorSet(i);
        }
    }

    void VulkanMaterial::UpdateDescriptorSet(uint32_t frameIndex)
    {
        if (!m_Context)
            return;
        if (frameIndex >= m_DescriptorSets.size())
            return;

        UpdateParamBuffer(frameIndex);
        if (frameIndex < m_ParamUBOs.size() && m_ParamUBOs[frameIndex].IsValid())
        {
            m_DescriptorSets[frameIndex].WriteUniformBuffer(ParamsBinding, m_ParamUBOs[frameIndex]);
        }

        const Ref<VulkanTexture>& albedoTexture = FirstValidTexture({
            &m_AlbedoTexture,
            &m_FallbackWhiteTexture
            });
        const Ref<VulkanTexture>& normalTexture = FirstValidTexture({
            &m_NormalTexture,
            &m_FallbackNormalTexture,
            &m_FallbackWhiteTexture,
            &m_AlbedoTexture
            });
        const Ref<VulkanTexture>& metallicRoughnessTexture = FirstValidTexture({
            &m_MetallicRoughnessTexture,
            &m_FallbackWhiteTexture,
            &m_AlbedoTexture
            });
        const Ref<VulkanTexture>& ambientOcclusionTexture = FirstValidTexture({
            &m_AmbientOcclusionTexture,
            &m_FallbackWhiteTexture,
            &m_AlbedoTexture
            });
        const Ref<VulkanTexture>& emissiveTexture = FirstValidTexture({
            &m_EmissiveTexture,
            &m_FallbackBlackTexture,
            &m_FallbackWhiteTexture,
            &m_AlbedoTexture
            });
        const Ref<VulkanTexture>& opacityTexture = FirstValidTexture({
            &m_OpacityTexture,
            &m_FallbackWhiteTexture,
            &m_AlbedoTexture
            });

        KITA_CORE_ASSERT(albedoTexture && albedoTexture->IsValid(), "VulkanMaterial requires a valid albedo or white fallback texture");
        KITA_CORE_ASSERT(normalTexture && normalTexture->IsValid(), "VulkanMaterial requires a valid normal-compatible fallback texture");
        KITA_CORE_ASSERT(metallicRoughnessTexture && metallicRoughnessTexture->IsValid(), "VulkanMaterial requires a valid metallic-roughness fallback texture");
        KITA_CORE_ASSERT(ambientOcclusionTexture && ambientOcclusionTexture->IsValid(), "VulkanMaterial requires a valid ambient-occlusion fallback texture");
        KITA_CORE_ASSERT(emissiveTexture && emissiveTexture->IsValid(), "VulkanMaterial requires a valid emissive fallback texture");
        KITA_CORE_ASSERT(opacityTexture && opacityTexture->IsValid(), "VulkanMaterial requires a valid opacity fallback texture");

        m_DescriptorSets[frameIndex].WriteImageSampler(AlbedoBinding, albedoTexture->GetDescriptorInfo());
        m_DescriptorSets[frameIndex].WriteImageSampler(NormalBinding, normalTexture->GetDescriptorInfo());
        m_DescriptorSets[frameIndex].WriteImageSampler(MetallicRoughnessBinding, metallicRoughnessTexture->GetDescriptorInfo());
        m_DescriptorSets[frameIndex].WriteImageSampler(AmbientOcclusionBinding, ambientOcclusionTexture->GetDescriptorInfo());
        m_DescriptorSets[frameIndex].WriteImageSampler(EmissiveBinding, emissiveTexture->GetDescriptorInfo());
        m_DescriptorSets[frameIndex].WriteImageSampler(OpacityBinding, opacityTexture->GetDescriptorInfo());

        if (frameIndex < m_DescriptorDirtyFlags.size())
        {
            m_DescriptorDirtyFlags[frameIndex] = 0;
        }
    }

    void VulkanMaterial::MarkDescriptorSetsDirty()
    {
        if (m_DescriptorDirtyFlags.empty())
            return;

        std::fill(m_DescriptorDirtyFlags.begin(), m_DescriptorDirtyFlags.end(), static_cast<uint8_t>(1));
    }

    void VulkanMaterial::EnsureDescriptors(VulkanContext& context, uint32_t framesInFlight)
    {
        const uint32_t descriptorCount = std::max(1u, framesInFlight);
        if (m_Context != &context || m_DescriptorSets.size() != descriptorCount)
        {
            InitDescriptors(context, descriptorCount);
            return;
        }

        if (m_DescriptorSets.empty())
        {
            InitDescriptors(context, descriptorCount);
        }
    }

    bool VulkanMaterial::HasAnyTexture() const
    {
        return
            (m_AlbedoTexture && m_AlbedoTexture->IsValid()) ||
            (m_NormalTexture && m_NormalTexture->IsValid()) ||
            (m_MetallicRoughnessTexture && m_MetallicRoughnessTexture->IsValid()) ||
            (m_AmbientOcclusionTexture && m_AmbientOcclusionTexture->IsValid()) ||
            (m_EmissiveTexture && m_EmissiveTexture->IsValid()) ||
            (m_OpacityTexture && m_OpacityTexture->IsValid());
    }

    bool VulkanMaterial::IsDescriptorSetDirty(uint32_t frameIndex) const
    {
        if (frameIndex >= m_DescriptorDirtyFlags.size())
            return false;

        return m_DescriptorDirtyFlags[frameIndex] != 0;
    }

    void VulkanMaterial::DestroyDescriptorResources()
    {
        for (auto& ubo : m_ParamUBOs)
            ubo.Destroy();
        m_ParamUBOs.clear();

        for (auto& descriptorSet : m_DescriptorSets)
            descriptorSet.Destroy();
        m_DescriptorSets.clear();
        m_DescriptorDirtyFlags.clear();

        m_Context = nullptr;
    }

    void VulkanMaterial::Destroy()
    {
        DestroyDescriptorResources();

        m_VertexShader = nullptr;
        m_FragmentShader = nullptr;
        m_AlbedoTexture = nullptr;
        m_NormalTexture = nullptr;
        m_MetallicRoughnessTexture = nullptr;
        m_AmbientOcclusionTexture = nullptr;
        m_EmissiveTexture = nullptr;
        m_OpacityTexture = nullptr;
        m_FallbackWhiteTexture = nullptr;
        m_FallbackBlackTexture = nullptr;
        m_FallbackNormalTexture = nullptr;
    }

    const VulkanDescriptorSet& VulkanMaterial::GetDescriptorSet(uint32_t frameIndex) const
    {
        KITA_CORE_ASSERT(!m_DescriptorSets.empty(), "VulkanMaterial descriptor sets are empty");
        KITA_CORE_ASSERT(frameIndex < m_DescriptorSets.size(), "VulkanMaterial frame index is out of range");
        return m_DescriptorSets[frameIndex];
    }
}
