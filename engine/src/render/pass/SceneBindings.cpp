#include "kita_pch.h"
#include "SceneBindings.h"
#include "render/VulkanContext.h"

#include "core/Core.h"
#include "core/Log.h"
namespace Kita {


    SceneBindings::~SceneBindings()
    {
        Destroy();
    }

    void SceneBindings::Init(VulkanContext& context, uint32_t framesInFlight)
    {

        Destroy();
        const uint32_t frameCount = std::max(1u, framesInFlight);

        m_Context = &context;

        m_CameraUBOs.resize(frameCount);
        m_MainLightUBOs.resize(frameCount);
        m_DescriptorSets.resize(frameCount);

        VulkanDescriptorSet::CreateInfo descInfo{};

        descInfo.Name = "SceneBindings_Set";
        descInfo.Bindings = {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT },
            { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }
        };

        for (uint32_t i = 0; i < frameCount; ++i)
        {
            m_CameraUBOs[i].Init(context, sizeof(SceneCameraData), "SceneCameraUBO_" + std::to_string(i));
            m_MainLightUBOs[i].Init(context, sizeof(SceneDirectionalLightData), "SceneMainLightUBO_" + std::to_string(i));

            VulkanDescriptorSet::CreateInfo perFrameDesc = descInfo;
            perFrameDesc.Name += "_" + std::to_string(i);

            m_DescriptorSets[i].Init(context, perFrameDesc);
            m_DescriptorSets[i].WriteUniformBuffer(0, m_CameraUBOs[i]);
            m_DescriptorSets[i].WriteUniformBuffer(1, m_MainLightUBOs[i]);
        }

        m_Initialized = true;
    }

    void SceneBindings::Destroy()
    {
        for (auto& set : m_DescriptorSets)
            set.Destroy();

        for (auto& ubo : m_CameraUBOs)
            ubo.Destroy();

        for (auto& ubo : m_MainLightUBOs)
            ubo.Destroy();

        m_DescriptorSets.clear();
        m_CameraUBOs.clear();
        m_MainLightUBOs.clear();

        m_Context = nullptr;
        m_Initialized = false;
    }

    void SceneBindings::Update(uint32_t frameIndex, const SceneCameraData& camera, const SceneDirectionalLightData& mainLight)
    {
        KITA_CORE_ASSERT(m_Initialized, "SceneBindings is not initialized");
        KITA_CORE_ASSERT(frameIndex < m_CameraUBOs.size(), "SceneBindings frame index out of range");

        m_CameraUBOs[frameIndex].SetData(&camera, sizeof(SceneCameraData));
        m_MainLightUBOs[frameIndex].SetData(&mainLight, sizeof(SceneDirectionalLightData));
    }

    const VulkanDescriptorSet& SceneBindings::GetDescriptorSet(uint32_t frameIndex) const
    {
        KITA_CORE_ASSERT(m_Initialized, "SceneBindings is not initialized");
        KITA_CORE_ASSERT(frameIndex < m_DescriptorSets.size(), "SceneBindings frame index out of range");
        return m_DescriptorSets[frameIndex];
    }

}
