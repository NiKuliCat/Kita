#include "kita_pch.h"
#include "RenderContext.h"
#include "core/Core.h"
#include "core/Log.h"
#include "render/VulkanContext.h"
#include "render/VulkanRenderTargetView.h"
namespace Kita {
	RenderPassContext::RenderPassContext(VulkanContext& context, VkCommandBuffer commandBuffer, const VulkanRenderTargetView& rtView)
		:m_Context(&context), m_CommandBuffer(commandBuffer), m_RenderTargetView(rtView)
	{
		KITA_CORE_ASSERT(m_Context, "RenderPassContext context is null");
		KITA_CORE_ASSERT(m_CommandBuffer != VK_NULL_HANDLE, "RenderPassContext command buffer is null");
		KITA_CORE_ASSERT(m_RenderTargetView.IsValid(), "RenderPassContext render target view is invalid");
	}
	VulkanContext& RenderPassContext::GetContext() const
	{
		KITA_CORE_ASSERT(m_Context, "RenderPassContext context is null");
		return *m_Context;
	}
	uint32_t RenderPassContext::GetFrameIndex() const
	{
		return GetContext().GetCurrentFrameIndex();
	}
}
