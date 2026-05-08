#include "kita_pch.h"
#include "VulkanRenderer.h"

#include "VulkanContext.h"
#include "VulkanRenderTarget.h"
#include "VulkanGraphicsPipeline.h"
#include "VulkanGeometry.h"
#include "VulkanRenderCommand.h"
#include "core/Log.h"


namespace Kita {


    void VulkanRenderer::Init(VulkanContext& context)
    {
        if (m_Initialized)
            return;

        m_Context = &context;
        m_CameraUniformBuffer.Init(context, sizeof(CameraUBO), "Renderer_CameraUBO");

        VulkanDescriptorSet::CreateInfo descInfo{};
        descInfo.Name = "Renderer_CameraSet";
        descInfo.Bindings = {
            { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT }
        };

        m_CameraDescriptorSet.Init(context, descInfo);
        m_CameraDescriptorSet.WriteUniformBuffer(0, m_CameraUniformBuffer);

        m_Initialized = true;
    }

    void VulkanRenderer::OnDestroy()
    {
        m_CameraDescriptorSet.Destroy();
        m_CameraUniformBuffer.Destroy();
        m_Context = nullptr;
        m_Initialized = false;
    }

    void VulkanRenderer::BeginScene(VkCommandBuffer cmd, VulkanRenderTarget& renderTarget, const CameraUBO& camera, const glm::vec4& clearColor, float clearDepth, uint32_t clearStencil)
    {
        KITA_CORE_ASSERT(cmd != VK_NULL_HANDLE, "VulkanRenderer::BeginScene: command buffer is null");
        KITA_CORE_ASSERT(renderTarget.IsValid(), "VulkanRenderer::BeginScene: render target is invalid");

        m_CameraUBO = camera;
        m_CameraUniformBuffer.SetData(&m_CameraUBO, sizeof(CameraUBO));

        m_CurrentClearColor = clearColor;
        m_CurrentClearDepth = clearDepth;
        m_CurrentClearStencil = clearStencil;
        m_InScene = true;


        // Build clear values
        std::vector<VkClearValue> clearValues;
        const uint32_t colorCount = renderTarget.GetColorAttachmentCount();

        for (uint32_t i = 0; i < colorCount; ++i)
        {
            clearValues.push_back(VulkanRenderTarget::MakeColorClearValue(
                clearColor.r, clearColor.g, clearColor.b, clearColor.a));
        }

        VkClearValue depthClear{};
        const VkClearValue* depthClearPtr = nullptr;

        if (renderTarget.HasDepthAttachment())
        {
            depthClear = VulkanRenderTarget::MakeDepthClearValue(clearDepth, clearStencil);
            depthClearPtr = &depthClear;
        }

        renderTarget.BeginRendering(cmd, clearValues, depthClearPtr);
        VulkanRenderCommand::SetViewport(cmd, renderTarget.GetWidth(), renderTarget.GetHeight());
        VulkanRenderCommand::SetScissor(cmd, renderTarget.GetWidth(), renderTarget.GetHeight());
    }

    void VulkanRenderer::EndScene(
        VkCommandBuffer     cmd,
        VulkanRenderTarget& renderTarget)
    {
        KITA_CORE_ASSERT(cmd != VK_NULL_HANDLE, "VulkanRenderer::EndScene: command buffer is null");
        KITA_CORE_ASSERT(m_InScene, "VulkanRenderer::EndScene: not in scene");

        renderTarget.EndRendering(cmd, /*transitionSampledImages=*/true, /*transitionSampledDepth=*/false);
        m_InScene = false;
    }

    void VulkanRenderer::SubmitMesh(VkCommandBuffer cmd, VulkanGraphicsPipeline& pipeline, VulkanGeometry& geometry, const ObjectData& model)
    {

        KITA_CORE_ASSERT(cmd != VK_NULL_HANDLE, "VulkanRenderer::SubmitMesh: command buffer is null");
        KITA_CORE_ASSERT(pipeline.IsValid(), "VulkanRenderer::SubmitMesh: pipeline is invalid");
        KITA_CORE_ASSERT(geometry.GetVertexCount() > 0, "VulkanRenderer::SubmitMesh: geometry has no vertices");


        pipeline.Bind(cmd);
        m_CameraDescriptorSet.Bind(cmd, pipeline.GetLayout(), 0);
        PushObjectData(cmd, pipeline.GetLayout(), model);
        VulkanRenderCommand::BindGeometry(cmd, geometry);
        VulkanRenderCommand::DrawGeometry(cmd, geometry);
    }

    void VulkanRenderer::PushObjectData(VkCommandBuffer cmd, VkPipelineLayout layout, const ObjectData& objectData)
    {
        KITA_CORE_ASSERT(cmd != VK_NULL_HANDLE, "PushCameraAndModel: command buffer is null");
        KITA_CORE_ASSERT(layout != VK_NULL_HANDLE, "PushCameraAndModel: pipeline layout is null");

        vkCmdPushConstants(
            cmd, layout,
            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
            0,
            sizeof(ObjectData),
            &objectData);
    }


}
