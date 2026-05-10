#include "kita_pch.h"
#include "ForwardOpaquePass.h"
#include "render/VulkanRenderCommand.h"
#include "core/Core.h"
#include "core/Log.h"
namespace Kita {

    namespace
    {
        void PushObjectData(
            VkCommandBuffer cmd,
            VkPipelineLayout layout,
            const ObjectData& objectData)
        {
            KITA_CORE_ASSERT(cmd != VK_NULL_HANDLE, "PushObjectData command buffer is null");
            KITA_CORE_ASSERT(layout != VK_NULL_HANDLE, "PushObjectData pipeline layout is null");

            vkCmdPushConstants(
                cmd,
                layout,
                VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
                0,
                ObjectDataSize,
                &objectData);
        }

        bool IsValidDrawItem(const ForwardOpaqueDrawItem& item)
        {
            return item.Pipeline && item.Geometry && item.Material;
        }


    }

    ForwardOpaquePass::ForwardOpaquePass(SceneBindings& sceneBindings, RenderPassDesc desc)
        :SceneRenderPassBase(sceneBindings,std::move(desc))
    {

    }

    void ForwardOpaquePass::ClearDrawItems()
    {
        m_DrawItems.clear();
    }

    void ForwardOpaquePass::AddDrawItem(const ForwardOpaqueDrawItem& item)
    {
        m_DrawItems.push_back(item);
    }

    void ForwardOpaquePass::Execute(RenderPassContext& context)
    {
        const ScenePassData& sceneData = GetSceneData();

        BeginPass(context, sceneData.BeginInfo);
        UpdateSceneBindings(context);

        const uint32_t frameIndex = context.GetFrameIndex();
        const VulkanDescriptorSet& sceneSet = GetSceneBindings().GetDescriptorSet(frameIndex);
        VkCommandBuffer cmd = context.GetCommandBuffer();

        for (const ForwardOpaqueDrawItem& item : m_DrawItems)
        {
            if (!IsValidDrawItem(item))
                continue;

            if (!item.Pipeline->IsValid())
                continue;

            item.Pipeline->Bind(cmd);

            // set 0 = scene
            sceneSet.Bind(cmd, item.Pipeline->GetLayout(), 0);

            // set 1 = material
            if (item.Material->HasDescriptorSets())
            {
                item.Material->GetDescriptorSet(frameIndex).Bind(
                    cmd,
                    item.Pipeline->GetLayout(),
                    1);
            }

            PushObjectData(cmd, item.Pipeline->GetLayout(), item.PerObject);
            VulkanRenderCommand::BindGeometry(cmd, *item.Geometry);
            VulkanRenderCommand::DrawGeometry(cmd, *item.Geometry);

           
        }
        EndPass(context, sceneData.BeginInfo);
    }

    RenderPassDesc MakeForwardOpaquePassDesc(const VulkanRenderTarget& renderTarget)
    {
        RenderPassDesc desc{}; 
        desc.Name = "ForwardOpaquePass";
        desc.Type = PassType::ForwardOpaque;
        desc.Samples = VK_SAMPLE_COUNT_1_BIT;

        const uint32_t colorCount = renderTarget.GetColorAttachmentCount();
        desc.ColorFormats.reserve(colorCount);

        for (uint32_t i = 0; i < colorCount; ++i)
            desc.ColorFormats.push_back(renderTarget.GetColorFormat(i));

        if (renderTarget.HasDepthAttachment())
            desc.DepthFormat = renderTarget.GetDepthFormat();

        return desc;
    }

}