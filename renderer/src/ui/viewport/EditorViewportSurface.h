#pragma once

#include <EngineCore.h>
#include <EngineRender.h>
namespace Kita {

	class VulkanBuffer;
	class VulkanImage;

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
			VkFormat GBufferBaseColorFormat = VK_FORMAT_R8G8B8A8_UNORM;
			VkFormat GBufferNormalFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
			VkFormat GBufferMaterialFormat = VK_FORMAT_R8G8B8A8_UNORM;
			VkFormat GBufferEmissiveFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
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

		bool IsValid() const { return m_FinalRenderTarget != nullptr; }

		void EnsureSize(uint32_t width, uint32_t height);
		void Resize(uint32_t width, uint32_t height);

		uint32_t GetWidth() const { return m_FinalRenderTarget ? m_FinalRenderTarget->GetWidth() : 0; }
		uint32_t GetHeight() const { return m_FinalRenderTarget ? m_FinalRenderTarget->GetHeight() : 0; }
		VkExtent2D GetExtent() const {
			if (!m_FinalRenderTarget)
				return {};

			return m_FinalRenderTarget->GetExtent();
		}

		ImTextureID GetTextureID() const { return m_TextureID; }
		VkFormat GetPickingFormat() const { return VK_FORMAT_R32_UINT; }
		VkFormat GetPickingDepthFormat() const { return m_CreateInfo.DepthFormat; }

		VulkanRenderTarget& GetGBufferRenderTarget();
		const VulkanRenderTarget& GetGBufferRenderTarget() const;
		VulkanRenderTarget& GetFinalRenderTarget();
		const VulkanRenderTarget& GetFinalRenderTarget() const;
		VulkanRenderTarget& GetRenderTarget();
		const VulkanRenderTarget& GetRenderTarget() const;
		VulkanRenderTarget& GetPickingRenderTarget();
		const VulkanRenderTarget& GetPickingRenderTarget() const;
		uint32_t ReadPickingPixel(uint32_t x, uint32_t y);

	private:
		void CreateRenderTargets();
		void CreatePickingResources();
		void DestroyPickingResources();
		void RecreateTextureID();
		void ReleaseTextureID();

	private:
		VulkanContext* m_Context = nullptr;
		CreateInfo m_CreateInfo{};

		Unique<VulkanRenderTarget> m_GBufferRenderTarget = nullptr;
		Unique<VulkanRenderTarget> m_FinalRenderTarget = nullptr;
		Unique<VulkanRenderTarget> m_PickingRenderTarget = nullptr;
		Unique<VulkanBuffer> m_PickingReadbackBuffer = nullptr;
		ImTextureID m_TextureID = 0;

	};
}
