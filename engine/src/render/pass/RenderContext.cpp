#include "kita_pch.h"
#include "RenderContext.h"
#include "core/Core.h"
#include "core/Log.h"
#include "render/VulkanContext.h"
#include "render/VulkanRenderTarget.h"
namespace Kita {
	RenderPassContext::RenderPassContext(VulkanContext& context, VkCommandBuffer commandBuffer, VulkanRenderTarget& rt)
		:m_Context(&context), m_CommandBuffer(commandBuffer), m_RenderTarget(&rt)
	{
		KITA_CORE_ASSERT(m_Context, "RenderPassContext context is null");
		KITA_CORE_ASSERT(m_CommandBuffer != VK_NULL_HANDLE, "RenderPassContext command buffer is null");
		KITA_CORE_ASSERT(m_RenderTarget, "RenderPassContext render target is null");
	}
	VulkanContext& RenderPassContext::GetContext() const
	{
		KITA_CORE_ASSERT(m_Context, "RenderPassContext context is null");
		return *m_Context;
	}
	VulkanRenderTarget& RenderPassContext::GetRenderTarget() const
	{
		KITA_CORE_ASSERT(m_RenderTarget, "RenderPassContext render target is null");
		return *m_RenderTarget;
	}
	uint32_t RenderPassContext::GetFrameIndex() const
	{
		return GetContext().GetCurrentFrameIndex();
	}
}