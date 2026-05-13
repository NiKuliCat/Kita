#include "renderer_pch.h"
#include "EditorViewportSurface.h"

#include <backends/imgui_impl_vulkan.h>


namespace Kita {

	namespace
	{
		VulkanRenderTarget::CreateInfo BuildRenderTargetCreateInfo( const EditorViewportSurface::CreateInfo& createInfo )
		{
			VulkanRenderTarget::CreateInfo rtInfo{};
			rtInfo.Name = createInfo.Name;
			rtInfo.Width = createInfo.Width;
			rtInfo.Height = createInfo.Height;
			rtInfo.Samples = createInfo.Samples;

			VulkanRenderTarget::ColorAttachmentDesc colorAttachment{};
			colorAttachment.Name = createInfo.Name + "_Color";
			colorAttachment.Format = createInfo.ColorFormat;
			colorAttachment.CreateSampler = true;
			colorAttachment.CreateResolveImage = false;
			colorAttachment.Filter = createInfo.SamplerFilter;
			colorAttachment.AddressMode = createInfo.SamplerAddressMode;
			rtInfo.ColorAttachments.push_back(colorAttachment);

			rtInfo.DepthAttachment.Enabled = true;
			rtInfo.DepthAttachment.Name = createInfo.Name + "_Depth";
			rtInfo.DepthAttachment.Format = createInfo.DepthFormat;
			rtInfo.DepthAttachment.CreateSampler = false;

			return rtInfo;
		}


	}

	EditorViewportSurface::EditorViewportSurface(VulkanContext& context,const CreateInfo& createInfo)
	{
		Init(context, createInfo);
	}

	EditorViewportSurface::~EditorViewportSurface()
	{
		Destroy();
	}

	void EditorViewportSurface::Init(VulkanContext& context, const CreateInfo& createInfo)
	{
		Destroy();

		m_Context = &context;
		m_CreateInfo = createInfo;
		m_CreateInfo.Width = std::max(1u, m_CreateInfo.Width);
		m_CreateInfo.Height = std::max(1u, m_CreateInfo.Height);

		CreateRenderTarget();
		RecreateTextureID();
	}

	void EditorViewportSurface::Destroy()
	{
		if (m_Context)
		{
			m_Context->WaitIdle();
		}

		ReleaseTextureID();
		m_RenderTarget.reset();
		m_Context = nullptr;
		m_CreateInfo = {};
	}

	void EditorViewportSurface::EnsureSize(uint32_t width, uint32_t height)
	{
		width = std::max(1u, width);
		height = std::max(1u, height);

		if (!m_RenderTarget)
			return;

		if (width == m_RenderTarget->GetWidth() && height == m_RenderTarget->GetHeight())
			return;

		Resize(width, height);
	}


	void EditorViewportSurface::Resize(uint32_t width, uint32_t height)
	{
		KITA_CORE_ASSERT(m_Context, "EditorViewportSurface context is null");
		KITA_CORE_ASSERT(m_RenderTarget, "EditorViewportSurface render target is null");

		width = std::max(1u, width);
		height = std::max(1u, height);

		if (width == m_RenderTarget->GetWidth() && height == m_RenderTarget->GetHeight())
			return;

		m_Context->WaitIdle();

		ReleaseTextureID();

		m_CreateInfo.Width = width;
		m_CreateInfo.Height = height;
		m_RenderTarget->Resize(width, height);

		RecreateTextureID();
	}

	VulkanRenderTarget& EditorViewportSurface::GetRenderTarget()
	{
		KITA_CORE_ASSERT(m_RenderTarget, "EditorViewportSurface render target is null");
		return *m_RenderTarget;
	}

	const VulkanRenderTarget& EditorViewportSurface::GetRenderTarget() const
	{
		KITA_CORE_ASSERT(m_RenderTarget, "EditorViewportSurface render target is null");
		return *m_RenderTarget;
	}

	void EditorViewportSurface::CreateRenderTarget()
	{
		KITA_CORE_ASSERT(m_Context, "EditorViewportSurface context is null");

		VulkanRenderTarget::CreateInfo rtInfo = BuildRenderTargetCreateInfo(m_CreateInfo);
		m_RenderTarget = CreateUnique<VulkanRenderTarget>(*m_Context, rtInfo);
	}

	void EditorViewportSurface::RecreateTextureID()
	{
		ReleaseTextureID();

		if (!m_RenderTarget)
			return;

		const VulkanImage& sampledImage = m_RenderTarget->GetSampledColorAttachment(0);
		KITA_CORE_ASSERT(sampledImage.HasSampler(), "EditorViewportSurface sampled image has no sampler");
		KITA_CORE_ASSERT(sampledImage.GetView() != VK_NULL_HANDLE, "EditorViewportSurface sampled image view is null");

		m_TextureID = static_cast<ImTextureID>(reinterpret_cast<uint64_t>(
			ImGui_ImplVulkan_AddTexture(
				sampledImage.GetSampler(),
				sampledImage.GetView(),
				VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)));
	}

	void EditorViewportSurface::ReleaseTextureID()
	{
		if (!m_TextureID)
			return;

		ImGui_ImplVulkan_RemoveTexture(
			reinterpret_cast<VkDescriptorSet>(static_cast<uint64_t>(m_TextureID)));
		m_TextureID = 0;
	}

}