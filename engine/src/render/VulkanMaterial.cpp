#include "kita_pch.h"
#include "VulkanMaterial.h"
#include "VulkanContext.h"
#include "VulkanTexture.h"
#include "core/Log.h"

namespace Kita {
    VulkanMaterial::VulkanMaterial(const Ref<VulkanShader>& vertex, const Ref<VulkanShader>& frag, const Ref<VulkanTexture>& tex)
        :m_VertexShader(vertex),m_FragmentShader(frag),m_AlbedoTexture(tex)
    {
    }

    VulkanMaterial::~VulkanMaterial()
    {
        Destroy();
    }

    void VulkanMaterial::InitDescriptors(VulkanContext& context, uint32_t framesInFlight)
    {
        Destroy();

        m_Context = &context;
        const uint32_t descriptorCount = std::max(1u, framesInFlight);
        m_DescriptorSets.resize(descriptorCount);
        m_DescriptorDirtyFlags.assign(descriptorCount, 1);

        VulkanDescriptorSet::CreateInfo descInfo{};
        descInfo.Name = "Material_Set";
        descInfo.Bindings = {
            { 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
        };

        for (uint32_t i = 0; i < descriptorCount; ++i)
        {
            VulkanDescriptorSet::CreateInfo perFrameDescInfo = descInfo;
            perFrameDescInfo.Name += "_" + std::to_string(i);
            m_DescriptorSets[i].Init(context, perFrameDescInfo);
        }

        UpdateDescriptorSets();
    }

    void VulkanMaterial::UpdateDescriptorSets()
    {
        if (!m_Context || !m_AlbedoTexture)
            return;

        for (uint32_t i = 0; i < static_cast<uint32_t>(m_DescriptorSets.size()); ++i)
        {
            UpdateDescriptorSet(i);
        }
    }

    void VulkanMaterial::UpdateDescriptorSet(uint32_t frameIndex)
    {
        if (!m_Context || !m_AlbedoTexture)
            return;
        if (frameIndex >= m_DescriptorSets.size())
            return;

        const VkDescriptorImageInfo imageInfo = m_AlbedoTexture->GetDescriptorInfo();
        m_DescriptorSets[frameIndex].WriteImageSampler(0, imageInfo);
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

    bool VulkanMaterial::IsDescriptorSetDirty(uint32_t frameIndex) const
    {
        if (frameIndex >= m_DescriptorDirtyFlags.size())
            return false;

        return m_DescriptorDirtyFlags[frameIndex] != 0;
    }

    void VulkanMaterial::Destroy()
    {
        for (auto& descriptorSet : m_DescriptorSets)
            descriptorSet.Destroy();
        m_DescriptorSets.clear();
        m_DescriptorDirtyFlags.clear();
        m_Context = nullptr;
    }

    const VulkanDescriptorSet& VulkanMaterial::GetDescriptorSet(uint32_t frameIndex) const
    {
        KITA_CORE_ASSERT(!m_DescriptorSets.empty(), "VulkanMaterial descriptor sets are empty");
        KITA_CORE_ASSERT(frameIndex < m_DescriptorSets.size(), "VulkanMaterial frame index is out of range");
        return m_DescriptorSets[frameIndex];
    }
}
