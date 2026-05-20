#include "kita_pch.h"
#include "DeferredLightingPass.h"

#include "render/VulkanContext.h"
#include "render/VulkanGraphicsPipeline.h"
#include "render/VulkanRenderTarget.h"

namespace Kita {

	DeferredLightingPass::DeferredLightingPass(SceneBindings& sceneBindings, RenderPassDesc desc)
		: FullscreenPassBase(sceneBindings, std::move(desc))
	{
	}

	DeferredLightingPass::~DeferredLightingPass()
	{
		Destroy();
	}

	void DeferredLightingPass::Init(VulkanContext& context, uint32_t framesInFlight)
	{
		InitDescriptorSets(context, framesInFlight);
	}

	void DeferredLightingPass::Destroy()
	{
		for (auto& descriptorSet : m_DescriptorSets)
		{
			descriptorSet.Destroy();
		}
		m_DescriptorSets.clear();
		m_Context = nullptr;
		m_GBufferRenderTarget = nullptr;
	}

	void DeferredLightingPass::SetGBufferInput(const VulkanRenderTarget* gbufferRenderTarget)
	{
		m_GBufferRenderTarget = gbufferRenderTarget;
	}

	void DeferredLightingPass::UpdateFrameResources(uint32_t frameIndex)
	{
		UpdateDescriptorSet(frameIndex);
	}

	void DeferredLightingPass::BindAdditionalResources(
		RenderPassContext& context,
		VkCommandBuffer commandBuffer,
		const VulkanGraphicsPipeline& pipeline,
		uint32_t frameIndex)
	{
		(void)context;

		if (frameIndex >= m_DescriptorSets.size())
			return;

		m_DescriptorSets[frameIndex].Bind(commandBuffer, pipeline.GetLayout(), 1);
	}

	void DeferredLightingPass::InitDescriptorSets(VulkanContext& context, uint32_t framesInFlight)
	{
		Destroy();

		m_Context = &context;
		const uint32_t frameCount = std::max(1u, framesInFlight);
		m_DescriptorSets.resize(frameCount);

		VulkanDescriptorSet::CreateInfo descInfo{};
		descInfo.Name = "DeferredLightingGBuffer_Set";
		descInfo.Bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT },
			{ 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT }
		};

		for (uint32_t i = 0; i < frameCount; ++i)
		{
			VulkanDescriptorSet::CreateInfo perFrameDescInfo = descInfo;
			perFrameDescInfo.Name += "_" + std::to_string(i);
			m_DescriptorSets[i].Init(context, perFrameDescInfo);
		}
	}

	void DeferredLightingPass::UpdateDescriptorSet(uint32_t frameIndex)
	{
		if (!m_Context || !m_GBufferRenderTarget)
			return;
		if (frameIndex >= m_DescriptorSets.size())
			return;

		m_DescriptorSets[frameIndex].WriteImageSampler(0, m_GBufferRenderTarget->GetSampledColorDescriptorInfo(0));
		m_DescriptorSets[frameIndex].WriteImageSampler(1, m_GBufferRenderTarget->GetSampledColorDescriptorInfo(1));
		m_DescriptorSets[frameIndex].WriteImageSampler(2, m_GBufferRenderTarget->GetSampledColorDescriptorInfo(2));
		m_DescriptorSets[frameIndex].WriteImageSampler(3, m_GBufferRenderTarget->GetSampledColorDescriptorInfo(3));
		m_DescriptorSets[frameIndex].WriteImageSampler(4, m_GBufferRenderTarget->GetDepthDescriptorInfo());
	}

	RenderPassDesc MakeDeferredLightingPassDesc(const VulkanRenderTarget& renderTarget)
	{
		RenderPassDesc desc{};
		desc.Name = "DeferredLightingPass";
		desc.Type = PassType::DeferredLighting;
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
