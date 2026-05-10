#pragma once
#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <vulkan/vulkan.h>

namespace Kita {

    enum class PassType
    {
        Unknown = 0,
        ForwardOpaque,
        ForwardTransparent,
        PostProcess,
        GBuffer,
        DeferredLighting,
        ShadowCaster,
        UI
    };
    struct RenderPassDesc
    {
        std::string Name;
        PassType Type = PassType::Unknown;

        std::vector<VkFormat> ColorFormats;
        VkFormat DepthFormat = VK_FORMAT_UNDEFINED;
        VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;
    };

    struct RenderPassBeginInfo
    {
        glm::vec4 ClearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        float ClearDepth = 1.0f;
        uint32_t ClearStencil = 0;

        bool TransitionSampledColors = true;
        bool TransitionSampledDepth = false;
    };

    struct alignas(16) SceneCameraData
    {
        glm::mat4 Matrix_V;
        glm::mat4 Matrix_P;
        glm::mat4 Matrix_VP;
        glm::mat4 Matrix_I_V;
        glm::mat4 Matrix_I_P;
        glm::mat4 Matrix_I_VP;
        glm::vec4 CameraPosWS;
    };

    struct alignas(16) SceneDirectionalLightData
    {
        glm::vec4 Direction;
        glm::vec4 Color; // w = intensity
    };

    struct ScenePassData
    {
        SceneCameraData Camera{};
        SceneDirectionalLightData MainLight{};
        RenderPassBeginInfo BeginInfo{};
    };

    struct alignas(16) ObjectData
    {
        glm::mat4 Matrix_M;
        glm::mat4 Matrix_I_M;
    };

    static constexpr uint32_t ObjectDataSize = sizeof(ObjectData);

}