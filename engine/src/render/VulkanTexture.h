#pragma once

#include "core/Core.h"
#include "render/VulkanImage.h"
#include <vulkan/vulkan.h>

namespace Kita {

    class VulkanContext;

    enum class TextureType
    {
        None = 0,
        Texture2D,
        TextureCube
    };


    class VulkanTexture
    {
    public:
        struct CreateInfo
        {
            std::string Name;
            TextureType Type = TextureType::Texture2D;

            uint32_t Width = 1;
            uint32_t Height = 1;

            VkFormat Format = VK_FORMAT_R8G8B8A8_UNORM;
            bool CreateSampler = true;

            VkFilter Filter = VK_FILTER_LINEAR;
            VkSamplerAddressMode AddressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            float MaxAnisotropy = 1.0f;

            bool EnableMipmaps = false;
            uint32_t MipLevels = 1;

            const void* PixelData = nullptr;
            VkDeviceSize PixelDataSize = 0;
        };

    public:
        VulkanTexture() = default;
        VulkanTexture(VulkanContext& context, const CreateInfo& createInfo);
        ~VulkanTexture();

        VulkanTexture(const VulkanTexture&) = delete;
        VulkanTexture& operator=(const VulkanTexture&) = delete;

        VulkanTexture(VulkanTexture&& other) noexcept;
        VulkanTexture& operator=(VulkanTexture&& other) noexcept;

        void Init(VulkanContext& context, const CreateInfo& createInfo);
        void Destroy();

        bool IsValid() const { return m_Image.IsValid(); }

        uint32_t GetWidth() const { return m_Width; }
        uint32_t GetHeight() const { return m_Height; }
        VkFormat GetFormat() const { return m_Format; }
        uint32_t GetMipLevels() const { return m_MipLevels; }
        TextureType GetType() const { return m_Type; }
        uint32_t GetArrayLayers() const { return m_ArrayLayers; }
        const std::string& GetName() const { return m_Name; }

        VulkanImage& GetImage() { return m_Image; }
        const VulkanImage& GetImage() const { return m_Image; }

        VkImage GetHandle() const { return m_Image.GetHandle(); }
        VkImageView GetView() const { return m_Image.GetView(); }
        VkSampler GetSampler() const { return m_Image.GetSampler(); }

        VkDescriptorImageInfo GetDescriptorInfo(
            VkImageLayout layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) const
        {
            return m_Image.GetDescriptorInfo(layout);
        }

    private:
        static uint32_t CalculateMipLevels(uint32_t width, uint32_t height);

    private:
        std::string m_Name;
        uint32_t m_Width = 0;
        uint32_t m_Height = 0;
        uint32_t m_MipLevels = 1;
        uint32_t m_ArrayLayers = 1;
        VkFormat m_Format = VK_FORMAT_UNDEFINED;
        TextureType m_Type = TextureType::None;

        VulkanImage m_Image;
    };

}

