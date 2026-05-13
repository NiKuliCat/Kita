#pragma once

#include <EngineCore.h>
#include <EngineRender.h>
namespace Kita {


	class EditorViewportSurface
	{
	public:
		struct CreateInfo
		{
			std::string Name = "EditorViewportSurface";
			uint32_t Width = 1280;
			uint32_t Height = 720;
			VkSampleCountFlagBits Samples = VK_SAMPLE_COUNT_1_BIT;

			VkFormat ColorFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
			VkFormat DepthFormat = VK_FORMAT_D32_SFLOAT;

			VkFilter SamplerFilter = VK_FILTER_LINEAR;
			VkSamplerAddressMode SamplerAddressMode = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
		};

	public:
		EditorViewportSurface() = default;
		EditorViewportSurface(VulkanContext& context, const CreateInfo& createInfo);
		~EditorViewportSurface();

		EditorViewportSurface(const EditorViewportSurface&) = delete;
		EditorViewportSurface& operator=(const EditorViewportSurface&) = delete;

		EditorViewportSurface(EditorViewportSurface&&) = delete;
		EditorViewportSurface& operator=(EditorViewportSurface&&) = delete;

		void Init(VulkanContext& context, const CreateInfo& createInfo);
		void Destroy();

		bool IsValid() const { return m_RenderTarget != nullptr; }

		void EnsureSize(uint32_t width, uint32_t height);
		void Resize(uint32_t width, uint32_t height);

		uint32_t GetWidth() const { return m_RenderTarget ? m_RenderTarget->GetWidth() : 0; }
		uint32_t GetHeight() const { return m_RenderTarget ? m_RenderTarget->GetHeight() : 0; }
		VkExtent2D GetExtent() const {
			if (!m_RenderTarget)
				return {};

			return m_RenderTarget->GetExtent();
		}

		ImTextureID GetTextureID() const { return m_TextureID; }

		VulkanRenderTarget& GetRenderTarget();
		const VulkanRenderTarget& GetRenderTarget() const;

	private:
		void CreateRenderTarget();
		void RecreateTextureID();
		void ReleaseTextureID();

	private:
		VulkanContext* m_Context = nullptr;
		CreateInfo m_CreateInfo{};

		Unique<VulkanRenderTarget> m_RenderTarget = nullptr;
		ImTextureID m_TextureID = 0;

	};
}