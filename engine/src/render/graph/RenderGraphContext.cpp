#include "kita_pch.h"
#include "RenderGraphContext.h"
#include "render/VulkanRenderTarget.h"
#include "core/Log.h"
namespace Kita {



	RenderGraphContext::RenderGraphContext(VulkanContext& context, VkCommandBuffer commandBuffer, const std::vector<RenderGraphResource>& resources)
		:m_Context(&context),m_CommandBuffer(commandBuffer),m_Resources(&resources)
	{
	}

	VulkanRenderTarget& RenderGraphContext::GetRenderTarget(RenderGraphResourceID id) const
	{
		KITA_CORE_ASSERT(m_Resources && id < m_Resources->size(), "RenderGraph resource id is invalid");

		const RenderGraphResource& resource = m_Resources->at(id);
		KITA_CORE_ASSERT(
			resource.Type == RenderGraphResourceType::ImportedRenderTarget,
			"RenderGraph resource is not an imported render target");

		KITA_CORE_ASSERT(resource.ImportedRenderTarget, "RenderGraph imported render target is null");

		return *resource.RT;
	}

}