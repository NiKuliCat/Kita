#include "kita_pch.h"
#include "VulkanTexture.h"

#include "render/VulkanContext.h"
#include "core/Log.h"

#include <algorithm>
#include <cmath>

namespace Kita {

    VulkanTexture::VulkanTexture(VulkanContext& context, const CreateInfo& createInfo)
    {
        Init(context, createInfo);
    }

    VulkanTexture::~VulkanTexture()
    {
        Destroy();
    }

    VulkanTexture::VulkanTexture(VulkanTexture&& other) noexcept
        : m_Name(std::move(other.m_Name))
        , m_Width(other.m_Width)
        , m_Height(other.m_Height)
        , m_MipLevels(other.m_MipLevels)
        , m_ArrayLayers(other.m_ArrayLayers)
        , m_Format(other.m_Format)
        , m_Type(other.m_Type)
        , m_Image(std::move(other.m_Image))
    {
        other.m_Width = 0;
        other.m_Height = 0;
        other.m_MipLevels = 1;
        other.m_ArrayLayers = 1;
        other.m_Format = VK_FORMAT_UNDEFINED;
        other.m_Type = TextureType::None;
    }

    VulkanTexture& VulkanTexture::operator=(VulkanTexture&& other) noexcept
    {
        if (this == &other)
            return *this;

        Destroy();

        m_Name = std::move(other.m_Name);
        m_Width = other.m_Width;
        m_Height = other.m_Height;
        m_MipLevels = other.m_MipLevels;
        m_ArrayLayers = other.m_ArrayLayers;
        m_Format = other.m_Format;
        m_Type = other.m_Type;
        m_Image = std::move(other.m_Image);

        other.m_Width = 0;
        other.m_Height = 0;
        other.m_MipLevels = 1;
        other.m_ArrayLayers = 1;
        other.m_Format = VK_FORMAT_UNDEFINED;
        other.m_Type = TextureType::None;

        return *this;
    }

    void VulkanTexture::Init(VulkanContext& context, const CreateInfo& createInfo)
    {
        Destroy();

        KITA_CORE_ASSERT(createInfo.Width > 0, "VulkanTexture width must be > 0");
        KITA_CORE_ASSERT(createInfo.Height > 0, "VulkanTexture height must be > 0");
        KITA_CORE_ASSERT(createInfo.Format != VK_FORMAT_UNDEFINED, "VulkanTexture format is invalid");

        m_Name = createInfo.Name.empty() ? "VulkanTexture" : createInfo.Name;
        m_Width = createInfo.Width;
        m_Height = createInfo.Height;
        m_Format = createInfo.Format;
        m_Type = createInfo.Type;

        KITA_CORE_ASSERT(
            m_Type == TextureType::Texture2D || m_Type == TextureType::TextureCube,
            "VulkanTexture type must be Texture2D or TextureCube");

        const bool isCubeTexture = m_Type == TextureType::TextureCube;
        m_ArrayLayers = isCubeTexture ? 6u : 1u;

        if (createInfo.EnableMipmaps)
            m_MipLevels = std::max(1u, CalculateMipLevels(createInfo.Width, createInfo.Height));
        else
            m_MipLevels = std::max(1u, createInfo.MipLevels);

        VulkanImage::CreateInfo imageInfo{};
        imageInfo.Name = m_Name;
        imageInfo.Type = VK_IMAGE_TYPE_2D;
        imageInfo.ViewType = isCubeTexture ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D;
        imageInfo.Format = createInfo.Format;
        imageInfo.Extent = { createInfo.Width, createInfo.Height, 1 };
        imageInfo.MipLevels = m_MipLevels;
        imageInfo.ArrayLayers = m_ArrayLayers;
        imageInfo.Samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.Tiling = VK_IMAGE_TILING_OPTIMAL;
        // 采样纹理默认同时具备 transfer dst / src 能力。
        // 这样后续可以继续用于 mip 生成、IBL 预计算或拷贝到其他纹理。
        imageInfo.Usage = VK_IMAGE_USAGE_SAMPLED_BIT
                        | VK_IMAGE_USAGE_TRANSFER_DST_BIT
                        | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        imageInfo.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        imageInfo.Flags = isCubeTexture ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

        imageInfo.CreateSampler = createInfo.CreateSampler;
        imageInfo.MinFilter = createInfo.Filter;
        imageInfo.MagFilter = createInfo.Filter;
        imageInfo.MipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        imageInfo.AddressModeU = isCubeTexture ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : createInfo.AddressMode;
        imageInfo.AddressModeV = isCubeTexture ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : createInfo.AddressMode;
        imageInfo.AddressModeW = isCubeTexture ? VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE : createInfo.AddressMode;
        imageInfo.MaxAnisotropy = createInfo.MaxAnisotropy;
        imageInfo.MinLod = 0.0f;
        imageInfo.MaxLod = static_cast<float>(m_MipLevels);

        // First version:
        // only supports direct upload into mip 0.
        // If mipLevels > 1 but you don't generate mipmaps yet, keep this disabled in actual use.
        if (createInfo.PixelData && createInfo.PixelDataSize > 0)
        {
            KITA_CORE_ASSERT(
                m_MipLevels == 1,
                "Current VulkanTexture implementation only supports initial upload for a single mip level");
            KITA_CORE_ASSERT(
                !isCubeTexture,
                "Current VulkanTexture implementation does not support direct initial upload for cubemaps");
            imageInfo.InitialData = createInfo.PixelData;
            imageInfo.InitialDataSize = createInfo.PixelDataSize;
        }

        m_Image.Init(context, imageInfo);

        KITA_CORE_INFO(
            "Created VulkanTexture '{0}' ({1}x{2}, type={3}, layers={4}, mipLevels={5})",
            m_Name,
            m_Width,
            m_Height,
            isCubeTexture ? "Cube" : "2D",
            m_ArrayLayers,
            m_MipLevels);
    }

    void VulkanTexture::Destroy()
    {
        m_Image.Destroy();
        m_Name.clear();
        m_Width = 0;
        m_Height = 0;
        m_MipLevels = 1;
        m_ArrayLayers = 1;
        m_Format = VK_FORMAT_UNDEFINED;
        m_Type = TextureType::None;
    }

    uint32_t VulkanTexture::CalculateMipLevels(uint32_t width, uint32_t height)
    {
        const uint32_t maxDimension = std::max(width, height);
        return static_cast<uint32_t>(std::floor(std::log2(static_cast<float>(maxDimension)))) + 1;
    }

}
