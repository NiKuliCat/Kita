#include "kita_pch.h"
#include "VulkanImage.h"

#include "VulkanContext.h"
#include "VulkanBuffer.h"
#include "core/Log.h"

namespace Kita {

    // ========================================================================
    // Internal helpers
    // ========================================================================
    namespace {

        void VKCheck(VkResult result, const char* message)
        {
            if (result != VK_SUCCESS)
            {
                KITA_CORE_ERROR("{0}, VkResult = {1}", message, static_cast<int32_t>(result));
                throw std::runtime_error(message);
            }
        }

        VkCommandBuffer BeginSingleTimeCommands(VulkanContext& context)
        {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = context.GetCommandPool();
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
            VKCheck(
                vkAllocateCommandBuffers(context.GetDevice(), &allocInfo, &commandBuffer),
                "Failed to allocate single-time command buffer");

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            VKCheck(
                vkBeginCommandBuffer(commandBuffer, &beginInfo),
                "Failed to begin single-time command buffer");

            return commandBuffer;
        }

        void EndSingleTimeCommands(VulkanContext& context, VkCommandBuffer commandBuffer)
        {
            VKCheck(
                vkEndCommandBuffer(commandBuffer),
                "Failed to end single-time command buffer");

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            VKCheck(
                vkQueueSubmit(context.GetGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE),
                "Failed to submit single-time command buffer");

            VKCheck(
                vkQueueWaitIdle(context.GetGraphicsQueue()),
                "Failed to wait for graphics queue idle");

            vkFreeCommandBuffers(context.GetDevice(), context.GetCommandPool(), 1, &commandBuffer);
        }

    } // anonymous namespace

    // ========================================================================
    // Lifecycle
    // ========================================================================

    VulkanImage::VulkanImage(VulkanContext& context, const CreateInfo& createInfo)
    {
        Init(context, createInfo);
    }

    VulkanImage::~VulkanImage()
    {
        Destroy();
    }

    VulkanImage::VulkanImage(VulkanImage&& other) noexcept
        : m_Context(other.m_Context)
        , m_Name(std::move(other.m_Name))
        , m_Image(other.m_Image)
        , m_Memory(other.m_Memory)
        , m_Format(other.m_Format)
        , m_Extent(other.m_Extent)
        , m_CurrentLayout(other.m_CurrentLayout)
        , m_Usage(other.m_Usage)
        , m_AspectFlags(other.m_AspectFlags)
        , m_ViewType(other.m_ViewType)
        , m_MemoryProperties(other.m_MemoryProperties)
        , m_MipLevels(other.m_MipLevels)
        , m_ArrayLayers(other.m_ArrayLayers)
        , m_Samples(other.m_Samples)
        , m_ImageView(other.m_ImageView)
        , m_Sampler(other.m_Sampler)
    {
        other.m_Context = nullptr;
        other.m_Image = VK_NULL_HANDLE;
        other.m_Memory = VK_NULL_HANDLE;
        other.m_ImageView = VK_NULL_HANDLE;
        other.m_Sampler = VK_NULL_HANDLE;
        other.m_ViewType = VK_IMAGE_VIEW_TYPE_2D;
        other.m_MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    VulkanImage& VulkanImage::operator=(VulkanImage&& other) noexcept
    {
        if (this == &other)
            return *this;

        Destroy();

        m_Context       = other.m_Context;
        m_Name          = std::move(other.m_Name);
        m_Image         = other.m_Image;
        m_Memory        = other.m_Memory;
        m_Format        = other.m_Format;
        m_Extent        = other.m_Extent;
        m_CurrentLayout = other.m_CurrentLayout;
        m_Usage         = other.m_Usage;
        m_AspectFlags   = other.m_AspectFlags;
        m_ViewType      = other.m_ViewType;
        m_MemoryProperties = other.m_MemoryProperties;
        m_MipLevels     = other.m_MipLevels;
        m_ArrayLayers   = other.m_ArrayLayers;
        m_Samples       = other.m_Samples;
        m_ImageView     = other.m_ImageView;
        m_Sampler       = other.m_Sampler;

        other.m_Context = nullptr;
        other.m_Image = VK_NULL_HANDLE;
        other.m_Memory = VK_NULL_HANDLE;
        other.m_ImageView = VK_NULL_HANDLE;
        other.m_Sampler = VK_NULL_HANDLE;
        other.m_ViewType = VK_IMAGE_VIEW_TYPE_2D;
        other.m_MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

        return *this;
    }

    void VulkanImage::Init(VulkanContext& context, const CreateInfo& createInfo)
    {
        Destroy();

        KITA_CORE_ASSERT(createInfo.Format != VK_FORMAT_UNDEFINED, "VulkanImage format must not be undefined");
        KITA_CORE_ASSERT(createInfo.Usage != 0, "VulkanImage usage must not be zero");
        KITA_CORE_ASSERT(createInfo.Extent.width  >= 1, "VulkanImage width must be >= 1");
        KITA_CORE_ASSERT(createInfo.Extent.height >= 1, "VulkanImage height must be >= 1");
        KITA_CORE_ASSERT(createInfo.Extent.depth  >= 1, "VulkanImage depth must be >= 1");
        KITA_CORE_ASSERT(createInfo.MipLevels >= 1, "VulkanImage mip levels must be >= 1");
        KITA_CORE_ASSERT(createInfo.ArrayLayers >= 1, "VulkanImage array layers must be >= 1");

        m_Context = &context;
        m_Name    = createInfo.Name;

        m_Format        = createInfo.Format;
        m_Extent        = createInfo.Extent;
        m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_Usage         = createInfo.Usage;
        m_AspectFlags   = createInfo.AspectFlags != 0 ? createInfo.AspectFlags : DeduceAspectFlags(createInfo.Format);
        m_ViewType      = createInfo.ViewType;
        m_MemoryProperties = createInfo.MemoryProperties;
        m_MipLevels     = createInfo.MipLevels;
        m_ArrayLayers   = createInfo.ArrayLayers;
        m_Samples       = createInfo.Samples;

        // 1. Create VkImage
        VkImageCreateInfo imageCI{};
        imageCI.sType         = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCI.imageType     = createInfo.Type;
        imageCI.format        = createInfo.Format;
        imageCI.extent        = createInfo.Extent;
        imageCI.mipLevels     = createInfo.MipLevels;
        imageCI.arrayLayers   = createInfo.ArrayLayers;
        imageCI.samples       = createInfo.Samples;
        imageCI.tiling        = createInfo.Tiling;
        imageCI.usage         = createInfo.Usage;
        imageCI.sharingMode   = createInfo.SharingMode;
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (createInfo.Type == VK_IMAGE_TYPE_3D)
        {
            imageCI.flags |= VK_IMAGE_CREATE_2D_ARRAY_COMPATIBLE_BIT;
        }

        CreateImage(imageCI);

        // 2. Allocate & bind memory
        AllocateAndBindMemory();

        // 3. Create ImageView
        CreateImageView();

        // 4. Create Sampler (optional)
        if (createInfo.CreateSampler)
        {
            CreateSampler(createInfo);
        }

        // 5. Upload initial data (optional)
        if (createInfo.InitialData && createInfo.InitialDataSize > 0)
        {
            VulkanBuffer::CreateInfo stagingCI{};
            stagingCI.Name             = m_Name + "_UploadStaging";
            stagingCI.Size             = createInfo.InitialDataSize;
            stagingCI.Usage            = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            stagingCI.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                       | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            stagingCI.InitialData      = createInfo.InitialData;
            stagingCI.InitialDataSize  = createInfo.InitialDataSize;

            VulkanBuffer staging(*m_Context, stagingCI);
            VkCommandBuffer cmd = BeginSingleTimeCommands(*m_Context);

            VkImageLayout finalLayout = (m_Usage & VK_IMAGE_USAGE_SAMPLED_BIT)
                ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
                : VK_IMAGE_LAYOUT_GENERAL;

            RecordUploadCommands(cmd, staging.GetHandle(), finalLayout, 0, 0);

            EndSingleTimeCommands(*m_Context, cmd);
            m_CurrentLayout = finalLayout;
        }

        KITA_CORE_INFO("Created VulkanImage '{0}' ({1}x{2})",
            m_Name, m_Extent.width, m_Extent.height);
    }

    void VulkanImage::Destroy()
    {
        if (!m_Context)
            return;

        VkDevice device = m_Context->GetDevice();
        if (device == VK_NULL_HANDLE)
        {
            m_Context = nullptr;
            return;
        }

        // Destroy order: sampler → imageView → image → memory
        if (m_Sampler != VK_NULL_HANDLE)
        {
            vkDestroySampler(device, m_Sampler, nullptr);
            m_Sampler = VK_NULL_HANDLE;
        }

        if (m_ImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device, m_ImageView, nullptr);
            m_ImageView = VK_NULL_HANDLE;
        }

        if (m_Image != VK_NULL_HANDLE)
        {
            vkDestroyImage(device, m_Image, nullptr);
            m_Image = VK_NULL_HANDLE;
        }

        if (m_Memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device, m_Memory, nullptr);
            m_Memory = VK_NULL_HANDLE;
        }

        m_Format        = VK_FORMAT_UNDEFINED;
        m_Extent        = {};
        m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_Usage         = 0;
        m_AspectFlags   = VK_IMAGE_ASPECT_COLOR_BIT;
        m_ViewType      = VK_IMAGE_VIEW_TYPE_2D;
        m_MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        m_MipLevels     = 1;
        m_ArrayLayers   = 1;
        m_Samples       = VK_SAMPLE_COUNT_1_BIT;
        m_Name.clear();
        m_Context = nullptr;
    }

    // ========================================================================
    // Private creation steps
    // ========================================================================

    void VulkanImage::CreateImage(VkImageCreateInfo& imageCI)
    {
        VKCheck(
            vkCreateImage(m_Context->GetDevice(), &imageCI, nullptr, &m_Image),
            "Failed to create VkImage");
    }

    void VulkanImage::AllocateAndBindMemory()
    {
        VkMemoryRequirements memRequirements{};
        vkGetImageMemoryRequirements(m_Context->GetDevice(), m_Image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize  = memRequirements.size;
        allocInfo.memoryTypeIndex = VulkanBuffer::FindMemoryType(
            *m_Context, memRequirements.memoryTypeBits,
            m_MemoryProperties);

        VKCheck(
            vkAllocateMemory(m_Context->GetDevice(), &allocInfo, nullptr, &m_Memory),
            "Failed to allocate VulkanImage memory");

        VKCheck(
            vkBindImageMemory(m_Context->GetDevice(), m_Image, m_Memory, 0),
            "Failed to bind VulkanImage memory");
    }

    void VulkanImage::CreateImageView()
    {
        VkImageViewCreateInfo viewCI{};
        viewCI.sType      = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.image      = m_Image;
        viewCI.viewType   = m_ViewType;
        viewCI.format     = m_Format;

        viewCI.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCI.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCI.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewCI.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

        viewCI.subresourceRange.aspectMask     = m_AspectFlags;
        viewCI.subresourceRange.baseMipLevel   = 0;
        viewCI.subresourceRange.levelCount     = m_MipLevels;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.subresourceRange.layerCount     = m_ArrayLayers;

        VKCheck(
            vkCreateImageView(m_Context->GetDevice(), &viewCI, nullptr, &m_ImageView),
            "Failed to create VkImageView");
    }

    void VulkanImage::CreateSampler(const CreateInfo& createInfo)
    {
        VkSamplerCreateInfo samplerCI{};
        samplerCI.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerCI.magFilter        = createInfo.MagFilter;
        samplerCI.minFilter        = createInfo.MinFilter;
        samplerCI.mipmapMode       = createInfo.MipmapMode;
        samplerCI.addressModeU     = createInfo.AddressModeU;
        samplerCI.addressModeV     = createInfo.AddressModeV;
        samplerCI.addressModeW     = createInfo.AddressModeW;
        samplerCI.mipLodBias       = 0.0f;
        samplerCI.anisotropyEnable = createInfo.MaxAnisotropy > 1.0f ? VK_TRUE : VK_FALSE;
        samplerCI.maxAnisotropy    = createInfo.MaxAnisotropy;
        samplerCI.compareEnable    = createInfo.CompareEnable ? VK_TRUE : VK_FALSE;
        samplerCI.compareOp        = createInfo.CompareOp;
        samplerCI.minLod           = createInfo.MinLod;
        samplerCI.maxLod           = createInfo.MaxLod;
        samplerCI.borderColor      = createInfo.BorderColor;
        samplerCI.unnormalizedCoordinates = VK_FALSE;

        VKCheck(
            vkCreateSampler(m_Context->GetDevice(), &samplerCI, nullptr, &m_Sampler),
            "Failed to create VkSampler");
    }

    // ========================================================================
    // Layout transitions
    // ========================================================================

    VkImageAspectFlags VulkanImage::DeduceAspectFlags(VkFormat format)
    {
        switch (format)
        {
        case VK_FORMAT_D16_UNORM:
        case VK_FORMAT_X8_D24_UNORM_PACK32:
        case VK_FORMAT_D32_SFLOAT:
            return VK_IMAGE_ASPECT_DEPTH_BIT;

        case VK_FORMAT_S8_UINT:
            return VK_IMAGE_ASPECT_STENCIL_BIT;

        case VK_FORMAT_D16_UNORM_S8_UINT:
        case VK_FORMAT_D24_UNORM_S8_UINT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;

        default:
            return VK_IMAGE_ASPECT_COLOR_BIT;
        }
    }

    VkAccessFlags VulkanImage::GetAccessMask(VkImageLayout layout)
    {
        switch (layout)
        {
        case VK_IMAGE_LAYOUT_UNDEFINED:
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return 0;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_ACCESS_TRANSFER_READ_BIT;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_ACCESS_TRANSFER_WRITE_BIT;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_ACCESS_SHADER_READ_BIT;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
                 | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                 | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
                 | VK_ACCESS_SHADER_READ_BIT;

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return 0;

        case VK_IMAGE_LAYOUT_GENERAL:
            return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        default:
            KITA_CORE_ASSERT(false, "Unsupported VkImageLayout for access mask");
            return 0;
        }
    }

    VkPipelineStageFlags VulkanImage::GetPipelineStage(VkImageLayout layout)
    {
        switch (layout)
        {
        case VK_IMAGE_LAYOUT_UNDEFINED:
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
                 | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT
                 | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT
                 | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;

        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        case VK_IMAGE_LAYOUT_GENERAL:
            return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        default:
            KITA_CORE_ASSERT(false, "Unsupported VkImageLayout for pipeline stage");
            return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
        }
    }

    void VulkanImage::TransitionImageLayout(
        VkCommandBuffer      cmd,
        VkImage              image,
        VkImageLayout        oldLayout,
        VkImageLayout        newLayout,
        VkImageAspectFlags   aspectMask,
        uint32_t             baseMipLevel,
        uint32_t             mipLevelCount,
        uint32_t             baseArrayLayer,
        uint32_t             arrayLayerCount,
        VkAccessFlags        customSrcAccessMask,
        VkPipelineStageFlags customSrcStage,
        VkAccessFlags        customDstAccessMask,
        VkPipelineStageFlags customDstStage)
    {
        if (oldLayout == newLayout && customSrcStage == 0 && customDstStage == 0)
            return;

        VkImageMemoryBarrier barrier{};
        barrier.sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout     = oldLayout;
        barrier.newLayout     = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image         = image;

        barrier.subresourceRange.aspectMask     = aspectMask;
        barrier.subresourceRange.baseMipLevel   = baseMipLevel;
        barrier.subresourceRange.levelCount     = mipLevelCount;
        barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
        barrier.subresourceRange.layerCount     = arrayLayerCount;

        // Compute src/dst access masks and pipeline stages
        VkAccessFlags        srcAccessMask = customSrcAccessMask;
        VkPipelineStageFlags srcStage      = customSrcStage;
        VkAccessFlags        dstAccessMask = customDstAccessMask;
        VkPipelineStageFlags dstStage      = customDstStage;

        if (srcAccessMask == 0 && srcStage == 0)
        {
            srcAccessMask = GetAccessMask(oldLayout);
            srcStage      = GetPipelineStage(oldLayout);
        }

        if (dstAccessMask == 0 && dstStage == 0)
        {
            dstAccessMask = GetAccessMask(newLayout);
            dstStage      = GetPipelineStage(newLayout);
        }

        // Handle special cases:
        // UNDEFINED → anything: make src stage as early as possible
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        // Anything → PRESENT: make dst stage as late as possible
        if (newLayout == VK_IMAGE_LAYOUT_PRESENT_SRC_KHR)
            dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        // DEPTH_STENCIL → SHADER_READ_ONLY: keep depth stages for src
        if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
            newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        }

        barrier.srcAccessMask = srcAccessMask;
        barrier.dstAccessMask = dstAccessMask;

        vkCmdPipelineBarrier(
            cmd,
            srcStage, dstStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier);
    }

    void VulkanImage::TransitionLayout(
        VkCommandBuffer cmd, VkImageLayout newLayout,
        VkImageAspectFlags aspectMask,
        uint32_t baseMipLevel, uint32_t mipLevelCount,
        uint32_t baseArrayLayer, uint32_t arrayLayerCount)
    {
        TransitionImageLayout(
            cmd, m_Image, m_CurrentLayout, newLayout,
            aspectMask,
            baseMipLevel, mipLevelCount,
            baseArrayLayer, arrayLayerCount);

        // Update tracked layout for the subresource range that was transitioned.
        // Simplification: track per-image, not per-subresource.
        // For full-subresource transitions, just update the tracked layout.
        if (baseMipLevel == 0 && mipLevelCount == m_MipLevels &&
            baseArrayLayer == 0 && arrayLayerCount == m_ArrayLayers)
        {
            m_CurrentLayout = newLayout;
        }
    }

    void VulkanImage::TransitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout)
    {
        TransitionLayout(cmd, newLayout,
            m_AspectFlags,
            0, m_MipLevels,
            0, m_ArrayLayers);
    }

    // ========================================================================
    // Data upload
    // ========================================================================

    void VulkanImage::UploadPixelData(
        VkCommandBuffer cmd, VkImageLayout finalLayout,
        const void* data, VkDeviceSize dataSize,
        uint32_t mipLevel, uint32_t arrayLayer)
    {
        KITA_CORE_ASSERT(data, "VulkanImage upload data is null");
        KITA_CORE_ASSERT(dataSize > 0, "VulkanImage upload data size must be > 0");
        KITA_CORE_ASSERT((m_Usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) != 0,
            "VulkanImage must have TRANSFER_DST usage for data uploads");

        VulkanBuffer::CreateInfo stagingCI{};
        stagingCI.Name             = m_Name + "_UploadStaging";
        stagingCI.Size             = dataSize;
        stagingCI.Usage            = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        stagingCI.MemoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                   | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        stagingCI.InitialData      = data;
        stagingCI.InitialDataSize  = dataSize;

        VulkanBuffer staging(*m_Context, stagingCI);
        RecordUploadCommands(cmd, staging.GetHandle(), finalLayout, mipLevel, arrayLayer);
        m_CurrentLayout = finalLayout;
    }

    void VulkanImage::RecordUploadCommands(
        VkCommandBuffer cmd, VkBuffer stagingBuffer,
        VkImageLayout finalLayout,
        uint32_t mipLevel, uint32_t arrayLayer)
    {
        KITA_CORE_ASSERT(stagingBuffer != VK_NULL_HANDLE, "VulkanImage staging buffer is invalid");
        KITA_CORE_ASSERT((m_Usage & VK_IMAGE_USAGE_TRANSFER_DST_BIT) != 0,
            "VulkanImage must have TRANSFER_DST usage for data uploads");

        // Step 2: Transition image to TRANSFER_DST_OPTIMAL
        TransitionLayout(cmd, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, m_AspectFlags,
            mipLevel, 1, arrayLayer, 1);

        // Step 3: Copy buffer to image
        VkBufferImageCopy region{};
        region.bufferOffset      = 0;
        region.bufferRowLength   = 0;  // tightly packed
        region.bufferImageHeight = 0;  // tightly packed

        region.imageSubresource.aspectMask     = m_AspectFlags;
        region.imageSubresource.mipLevel       = mipLevel;
        region.imageSubresource.baseArrayLayer = arrayLayer;
        region.imageSubresource.layerCount     = 1;

        region.imageOffset = { 0, 0, 0 };
        region.imageExtent = {
            std::max(1u, m_Extent.width  >> mipLevel),
            std::max(1u, m_Extent.height >> mipLevel),
            std::max(1u, m_Extent.depth  >> mipLevel)
        };

        vkCmdCopyBufferToImage(
            cmd,
            stagingBuffer,
            m_Image,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            1, &region);

        // Step 4: Transition to final layout (caller-specified)
        TransitionLayout(cmd, finalLayout, m_AspectFlags,
            mipLevel, 1, arrayLayer, 1);
    }

    // ========================================================================
    // Descriptor info
    // ========================================================================

    VkDescriptorImageInfo VulkanImage::GetDescriptorInfo(VkImageLayout samplerLayout) const
    {
        KITA_CORE_ASSERT(m_Sampler != VK_NULL_HANDLE, "VulkanImage has no sampler");
        KITA_CORE_ASSERT(m_ImageView != VK_NULL_HANDLE, "VulkanImage has no image view");

        VkDescriptorImageInfo info{};
        info.sampler     = m_Sampler;
        info.imageView   = m_ImageView;
        info.imageLayout = samplerLayout;
        return info;
    }

    // ========================================================================
    // Factory helpers
    // ========================================================================

    VulkanImage::CreateInfo VulkanImage::MakeDepthStencil(
        VkExtent2D extent,
        VkFormat format,
        VkSampleCountFlagBits samples,
        VkImageUsageFlags extraUsage)
    {
        CreateInfo ci{};
        ci.Name        = "DepthStencil";
        ci.Type        = VK_IMAGE_TYPE_2D;
        ci.Format      = format;
        ci.Extent      = { extent.width, extent.height, 1 };
        ci.MipLevels   = 1;
        ci.ArrayLayers = 1;
        ci.Samples     = samples;
        ci.Usage       = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | extraUsage;
        ci.AspectFlags = DeduceAspectFlags(format);
        ci.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        return ci;
    }

    VulkanImage::CreateInfo VulkanImage::MakeColorAttachment(
        VkExtent2D extent,
        VkFormat format,
        VkSampleCountFlagBits samples,
        VkImageUsageFlags extraUsage)
    {
        CreateInfo ci{};
        ci.Name        = "ColorAttachment";
        ci.Type        = VK_IMAGE_TYPE_2D;
        ci.Format      = format;
        ci.Extent      = { extent.width, extent.height, 1 };
        ci.MipLevels   = 1;
        ci.ArrayLayers = 1;
        ci.Samples     = samples;
        ci.Usage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT
                       | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
                       | extraUsage;
        ci.AspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
        ci.InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        return ci;
    }

    VulkanImage::CreateInfo VulkanImage::MakeTexture(
        VkExtent2D extent,
        VkFormat format,
        uint32_t mipLevels,
        VkFilter filter,
        VkSamplerAddressMode addressMode,
        float maxAnisotropy)
    {
        CreateInfo ci{};
        ci.Name           = "Texture";
        ci.Type           = VK_IMAGE_TYPE_2D;
        ci.Format         = format;
        ci.Extent         = { extent.width, extent.height, 1 };
        ci.MipLevels      = mipLevels;
        ci.ArrayLayers    = 1;
        ci.Samples        = VK_SAMPLE_COUNT_1_BIT;
        ci.Usage          = VK_IMAGE_USAGE_TRANSFER_DST_BIT
                          | VK_IMAGE_USAGE_SAMPLED_BIT;
        ci.AspectFlags    = VK_IMAGE_ASPECT_COLOR_BIT;
        ci.InitialLayout  = VK_IMAGE_LAYOUT_UNDEFINED;

        // Sampler
        ci.CreateSampler  = true;
        ci.MinFilter      = filter;
        ci.MagFilter      = filter;
        ci.AddressModeU   = addressMode;
        ci.AddressModeV   = addressMode;
        ci.AddressModeW   = addressMode;
        ci.MaxAnisotropy  = maxAnisotropy;
        ci.MaxLod         = static_cast<float>(mipLevels);

        return ci;
    }

}
