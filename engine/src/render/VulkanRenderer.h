#pragma once
#include "VulkanUniformBuffer.h"
#include "VulkanDescriptorSet.h"
#include "VulkanTexture.h"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>
#include <vector>
#include <utility>

namespace Kita {

    class VulkanContext;
    class VulkanRenderTarget;
    class VulkanGraphicsPipeline;
    class VulkanGeometry;

    // ========================================================================
    // VulkanRenderer — 场景渲染协调层，替代旧 OpenGL Renderer
    //
    // 封装 render target begin/end + push constant 传递 + 绘制命令。
    // 被 SceneViewportPanel 调用，管理层视图口的每帧渲染。
    //
    // 依赖：VulkanGraphicsPipeline 的 PipelineLayout 需声明
    //   VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT 的
    //   push constant range（至少 192 字节 = CameraData + PerObjectData）
    //
    // 典型用法：
    //   VulkanRenderer renderer;
    //   renderer.BeginScene(cmd, *rt, camera, clearColor);
    //   renderer.SubmitMesh(cmd, *pipeline, *geometry, modelMatrix);
    //   renderer.EndScene(cmd, *rt);
    // ========================================================================
    class VulkanRenderer
    {
    public:


        struct alignas(16) CameraUBO
        {
            glm::mat4 Matrix_V;
            glm::mat4 Matrix_P;
            glm::mat4 Matrix_VP;
            glm::mat4 Matrix_I_V;
            glm::mat4 Matrix_I_P;
            glm::mat4 Matrix_I_VP;
            glm::vec4 CameraPosWS;
        };

        struct alignas(16) DirectionLightUBO
        {
            glm::vec4 Direction;
            glm::vec4 Color; // w intensity
        };

        struct alignas(16) PointLightSSBO
        {
            glm::vec4 Position;
            glm::vec4 Color; // w intensity
            glm::vec4 Range;
        };

        struct alignas(16) ObjectData
        {
            glm::mat4 Matrix_M;
            glm::mat4 Matrix_I_M;
        };

        static const uint32_t ObjectDataSize = sizeof(ObjectData);

        // ---- 编辑器网格默认设置 ----
        struct EditorGridSettings
        {
            float CellSize     = 0.25f;
            float MajorStep    = 5.0f;
            float MinorWidthPx = 0.35f;
            float MajorWidthPx = 0.50f;
            float AxisWidthPx  = 0.65f;
            float FadeStart    = 30.0f;
            float FadeEnd      = 100.0f;

            glm::vec4 MinorColor = glm::vec4(0.73f, 0.75f, 0.78f, 0.55f);
            glm::vec4 MajorColor = glm::vec4(0.86f, 0.88f, 0.89f, 0.85f);
            glm::vec4 AxisXColor = glm::vec4(0.93f, 0.35f, 0.33f, 0.60f);
            glm::vec4 AxisZColor = glm::vec4(0.33f, 0.60f, 0.96f, 0.60f);
        };

    public:
        VulkanRenderer(VulkanContext& context)
        {
            Init(context);
        }

        void Init(VulkanContext& context);
        void OnDestroy();
        const VulkanDescriptorSet& GetSceneUniformDescriptorSet() const;

        void BeginScene(
            VkCommandBuffer        cmd,
            VulkanRenderTarget& renderTarget,
            const CameraUBO& camera,
            const DirectionLightUBO& dirLight,
            const glm::vec4& clearColor,
            float                  clearDepth = 1.0f,
            uint32_t               clearStencil = 0);

        // 结束渲染。自动：
        //   1. renderTarget.EndRendering(cmd, ...)
        void EndScene(
            VkCommandBuffer     cmd,
            VulkanRenderTarget& renderTarget);

        void SubmitMesh(
            VkCommandBuffer            cmd,
            VulkanGraphicsPipeline&    pipeline,
            VulkanGeometry&            geometry,
            const ObjectData&           model);



        void PushObjectData(
            VkCommandBuffer    cmd,
            VkPipelineLayout   layout,
            const ObjectData& objectData
        );
      

    private:
        bool m_Initialized = false;
        VulkanContext* m_Context = nullptr;
        CameraUBO  m_CameraUBO{};
        std::vector<VulkanUniformBuffer> m_DirLightUniformBuffers;
        std::vector<VulkanUniformBuffer> m_CameraUniformBuffers;
        std::vector<VulkanDescriptorSet> m_SceneUniformDescriptorSets;

        glm::vec4  m_CurrentClearColor{};
        float      m_CurrentClearDepth   = 1.0f;
        uint32_t   m_CurrentClearStencil = 0;
        bool       m_InScene = false;
    };

}
