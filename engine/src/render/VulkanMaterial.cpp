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

        const VkDescriptorImageInfo imageInfo = m_AlbedoTexture->GetDescriptorInfo();
        for (auto& descriptorSet : m_DescriptorSets)
            descriptorSet.WriteImageSampler(0, imageInfo);
    }

    void VulkanMaterial::Destroy()
    {
        for (auto& descriptorSet : m_DescriptorSets)
            descriptorSet.Destroy();
        m_DescriptorSets.clear();
        m_Context = nullptr;
    }

    const VulkanDescriptorSet& VulkanMaterial::GetDescriptorSet(uint32_t frameIndex) const
    {
        KITA_CORE_ASSERT(!m_DescriptorSets.empty(), "VulkanMaterial descriptor sets are empty");
        KITA_CORE_ASSERT(frameIndex < m_DescriptorSets.size(), "VulkanMaterial frame index is out of range");
        return m_DescriptorSets[frameIndex];
    }
}
