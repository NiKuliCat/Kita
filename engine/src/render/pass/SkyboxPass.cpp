#include "kita_pch.h"
#include "SkyboxPass.h"
#include "render/VulkanContext.h"
#include "render/VulkanGraphicsPipeline.h"
#include "render/VulkanTexture.h"

namespace Kita {



	SkyboxPass::SkyboxPass(SceneBindings& sceneBindings, RenderPassDesc desc)
		:FullscreenPassBase(sceneBindings,desc)
	{
	}

	bool SkyboxPass::HasValidMaterial() const
	{
		if (!m_Material)
		{
			return false;
		}

		if (!m_Material->GetVertexShader() || !m_Material->GetFragmentShader())
		{
			return false;
		}

		const Ref<VulkanTexture>& texture = m_Material->GetAlbedoTexture();
		if (!texture || !texture->IsValid())
		{
			return false;
		}

		return texture->GetType() == TextureType::TextureCube;
	}

	void SkyboxPass::BindAdditionalResources(RenderPassContext& context, VkCommandBuffer commandBuffer, const VulkanGraphicsPipeline& pipeline, uint32_t frameIndex)
	{
		(void)context;

		if (!HasValidMaterial())
		{
			return;
		}

		if (!m_Material->HasDescriptorSets())
		{
			return;
		}

		m_Material->GetDescriptorSet(frameIndex).Bind(commandBuffer,pipeline.GetLayout(),1);
	}

	RenderPassDesc MakeSkyboxPassDesc(const VulkanRenderTarget& renderTarget)
	{
		RenderPassDesc desc{};
		desc.Name = "SkyboxPass";
		desc.Type = PassType::PostProcess;
		desc.Samples = renderTarget.GetCreateInfo().Samples;
		desc.UseDepthAttachment = renderTarget.HasDepthAttachment();

		const uint32_t colorCount = renderTarget.GetColorAttachmentCount();
		desc.ColorFormats.reserve(colorCount);
		for (uint32_t i = 0; i < colorCount; ++i)
		{
			desc.ColorFormats.push_back(renderTarget.GetColorFormat(i));
		}

		if (renderTarget.HasDepthAttachment())
		{
			desc.DepthFormat = renderTarget.GetDepthFormat();
		}

		return desc;
	}

}