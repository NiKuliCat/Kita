#include "kita_pch.h"
#include "ToneMappingPass.h"
#include "render/VulkanContext.h"
#include "render/VulkanGraphicsPipeline.h"
#include "render/VulkanRenderTarget.h"
namespace Kita {



	ToneMappingPass::ToneMappingPass(SceneBindings& sceneBindings, RenderPassDesc desc)
		:FullscreenPassBase(sceneBindings,desc)
	{
	}

	ToneMappingPass::~ToneMappingPass()
	{
		Destroy();
	}

	void ToneMappingPass::Init(VulkanContext& context, uint32_t framesInFlight)
	{
		InitDescriptorSets(context, framesInFlight);
	}

	void ToneMappingPass::Destroy()
	{
		for (auto& descriptorSet : m_DescriptorSets)
		{
			descriptorSet.Destroy();
		}
		m_DescriptorSets.clear();
		m_Context = nullptr;
		m_SourceRenderTarget.Reset();
	}

	void ToneMappingPass::SetSourceInput(const VulkanRenderTargetView& sourceRenderTarget)
	{
		m_SourceRenderTarget = sourceRenderTarget;
	}

	void ToneMappingPass::UpdateFrameResources(uint32_t frameIndex)
	{
		UpdateDescriptorSet(frameIndex);
	}

	void ToneMappingPass::BindAdditionalResources(RenderPassContext& context, VkCommandBuffer commandBuffer, const VulkanGraphicsPipeline& pipeline, uint32_t frameIndex)
	{
		(void)context;

		if (frameIndex >= m_DescriptorSets.size())
			return;

		m_DescriptorSets[frameIndex].Bind(commandBuffer, pipeline.GetLayout(), 1);
	}

	void ToneMappingPass::InitDescriptorSets(VulkanContext& context, uint32_t framesInFlight)
	{
		Destroy();

		m_Context = &context;
		const uint32_t frameCount = std::max(1u, framesInFlight);
		m_DescriptorSets.resize(frameCount);

		VulkanDescriptorSet::CreateInfo descInfo{};
		descInfo.Name = "TonemapPass_Set";
		descInfo.Bindings = {
			{ 0, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, VK_SHADER_STAGE_FRAGMENT_BIT } // HDR source color
		};

		for (uint32_t i = 0; i < frameCount; ++i)
		{
			VulkanDescriptorSet::CreateInfo perFrameDescInfo = descInfo;
			perFrameDescInfo.Name += "_" + std::to_string(i);
			m_DescriptorSets[i].Init(context, perFrameDescInfo);
		}
	}

	void ToneMappingPass::UpdateDescriptorSet(uint32_t frameIndex)
	{
		if (!m_Context || !m_SourceRenderTarget.IsValid())
			return;
		if (frameIndex >= m_DescriptorSets.size())
			return;

		m_DescriptorSets[frameIndex].WriteImageSampler(
			0,
			m_SourceRenderTarget.GetSampledColorDescriptorInfo(0));
	}

	RenderPassDesc MakeTonemappingPassDesc(const VulkanRenderTargetView& renderTarget)
	{
		RenderPassDesc desc{};
		desc.Name = "TonemapPass";
		desc.Type = PassType::PostProcess;
		desc.Samples = renderTarget.GetSamples();
		desc.UseDepthAttachment = renderTarget.HasDepthAttachment();

		const uint32_t colorCount = renderTarget.GetColorAttachmentCount();
		desc.ColorFormats.reserve(colorCount);
		for (uint32_t i = 0; i < colorCount; ++i)
		{
			desc.ColorFormats.push_back(renderTarget.GetColorFormat(i));
		}

		if (renderTarget.HasDepthAttachment())
			desc.DepthFormat = renderTarget.GetDepthFormat();

		return desc;
	}

}
