#pragma once

#include <vulkan/vulkan.h>

namespace Kita {

    class VulkanContext;
    class VulkanBuffer;

    class VulkanImage
    {
    public:
        struct CreateInfo
        {
            std::string Name;

            // ---- Image ----
            VkImageType         Type          = VK_IMAGE_TYPE_2D;
            VkFormat            Format        = VK_FORMAT_UNDEFINED;
            VkExtent3D          Extent        = { 1, 1, 1 };
            uint32_t            MipLevels     = 1;
            uint32_t            ArrayLayers   = 1;
            VkSampleCountFlagBits Samples     = VK_SAMPLE_COUNT_1_BIT;
            VkImageTiling       Tiling        = VK_IMAGE_TILING_OPTIMAL;
            VkImageUsageFlags   Usage         = 0;
            VkImageLayout       InitialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            VkSharingMode       SharingMode   = VK_SHARING_MODE_EXCLUSIVE;
            VkImageCreateFlags  Flags         = 0;

            // ---- Memory ----
            VkMemoryPropertyFlags MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

            // ---- ImageView (auto-created) ----
            VkImageAspectFlags  AspectFlags    = VK_IMAGE_ASPECT_COLOR_BIT;
            VkImageViewType     ViewType       = VK_IMAGE_VIEW_TYPE_2D;

            // ---- Sampler (optional) ----
            bool                CreateSampler  = false;
            VkFilter            MinFilter      = VK_FILTER_LINEAR;
            VkFilter            MagFilter      = VK_FILTER_LINEAR;
            VkSamplerMipmapMode MipmapMode     = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            VkSamplerAddressMode AddressModeU  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            VkSamplerAddressMode AddressModeV  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            VkSamplerAddressMode AddressModeW  = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            float               MaxAnisotropy  = 1.0f;
            VkBorderColor       BorderColor    = VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            bool                CompareEnable  = false;
            VkCompareOp         CompareOp      = VK_COMPARE_OP_ALWAYS;
            float               MinLod         = 0.0f;
            float               MaxLod         = 1000.0f;

            // ---- Initial data upload (staging) ----
            const void*         InitialData     = nullptr;
            VkDeviceSize        InitialDataSize = 0;
        };

    public:
        // ---- Lifecycle ----
        VulkanImage() = default;
        VulkanImage(VulkanContext& context, const CreateInfo& createInfo);
        ~VulkanImage();

        VulkanImage(const VulkanImage&) = delete;
        VulkanImage& operator=(const VulkanImage&) = delete;

        VulkanImage(VulkanImage&& other) noexcept;
        VulkanImage& operator=(VulkanImage&& other) noexcept;

        void Init(VulkanContext& context, const CreateInfo& createInfo);
        void Destroy();

        bool IsValid() const { return m_Image != VK_NULL_HANDLE; }
        bool HasSampler() const { return m_Sampler != VK_NULL_HANDLE; }

        // ---- Layout transitions ----
        void TransitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout);
        void TransitionLayout(VkCommandBuffer cmd, VkImageLayout newLayout,
            VkImageAspectFlags aspectMask,
            uint32_t baseMipLevel, uint32_t mipLevelCount,
            uint32_t baseArrayLayer, uint32_t arrayLayerCount);

        // ---- Static: transition any VkImage (usable by swapchain code) ----
        static void TransitionImageLayout(
            VkCommandBuffer      cmd,
            VkImage              image,
            VkImageLayout        oldLayout,
            VkImageLayout        newLayout,
            VkImageAspectFlags   aspectMask,
            uint32_t             baseMipLevel   = 0,
            uint32_t             mipLevelCount  = 1,
            uint32_t             baseArrayLayer = 0,
            uint32_t             arrayLayerCount = 1,
            VkAccessFlags        customSrcAccessMask = 0,
            VkPipelineStageFlags customSrcStage      = 0,
            VkAccessFlags        customDstAccessMask = 0,
            VkPipelineStageFlags customDstStage      = 0);

        // ---- Data upload via staging buffer ----
        void UploadPixelData(VkCommandBuffer cmd, VkImageLayout finalLayout,
            const void* data, VkDeviceSize dataSize,
            uint32_t mipLevel = 0, uint32_t arrayLayer = 0);

        // ---- Accessors ----
        VulkanContext*       GetContext()       const { return m_Context; }
        VkImage              GetHandle()        const { return m_Image; }
        VkImageView          GetView()          const { return m_ImageView; }
        VkSampler            GetSampler()       const { return m_Sampler; }
        VkFormat             GetFormat()        const { return m_Format; }
        VkExtent3D           GetExtent()        const { return m_Extent; }
        VkImageLayout        GetCurrentLayout() const { return m_CurrentLayout; }
        VkImageAspectFlags   GetAspectFlags()   const { return m_AspectFlags; }
        uint32_t             GetMipLevels()     const { return m_MipLevels; }
        uint32_t             GetArrayLayers()   const { return m_ArrayLayers; }
        VkSampleCountFlagBits GetSamples()      const { return m_Samples; }
        VkImageUsageFlags    GetUsage()         const { return m_Usage; }
        const std::string&   GetName()          const { return m_Name; }

        VkDescriptorImageInfo GetDescriptorInfo(VkImageLayout samplerLayout) const;

        // ---- Factory helpers (convenience) ----
        static CreateInfo MakeDepthStencil(
            VkExtent2D extent,
            VkFormat format = VK_FORMAT_D32_SFLOAT,
            VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
            VkImageUsageFlags extraUsage = 0);

        static CreateInfo MakeColorAttachment(
            VkExtent2D extent,
            VkFormat format,
            VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT,
            VkImageUsageFlags extraUsage = 0);

        static CreateInfo MakeTexture(
            VkExtent2D extent,
            VkFormat format,
            uint32_t mipLevels = 1,
            VkFilter filter = VK_FILTER_LINEAR,
            VkSamplerAddressMode addressMode = VK_SAMPLER_ADDRESS_MODE_REPEAT,
            float maxAnisotropy = 1.0f);

    private:
        void CreateImage(VkImageCreateInfo& imageCI);
        void AllocateAndBindMemory();
        void CreateImageView();
        void CreateSampler(const CreateInfo& createInfo);
        void RecordUploadCommands(VkCommandBuffer cmd, VkBuffer stagingBuffer,
            VkImageLayout finalLayout, uint32_t mipLevel, uint32_t arrayLayer);

        static VkImageAspectFlags   DeduceAspectFlags(VkFormat format);
        static VkAccessFlags        GetAccessMask(VkImageLayout layout);
        static VkPipelineStageFlags GetPipelineStage(VkImageLayout layout);

    private:
        VulkanContext* m_Context = nullptr;
        std::string    m_Name;

        // Image
        VkImage        m_Image  = VK_NULL_HANDLE;
        VkDeviceMemory m_Memory = VK_NULL_HANDLE;
        VkFormat       m_Format = VK_FORMAT_UNDEFINED;
        VkExtent3D     m_Extent{};
        VkImageLayout  m_CurrentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkImageUsageFlags     m_Usage          = 0;
        VkImageAspectFlags    m_AspectFlags    = VK_IMAGE_ASPECT_COLOR_BIT;
        VkImageViewType       m_ViewType       = VK_IMAGE_VIEW_TYPE_2D;
        VkMemoryPropertyFlags m_MemoryProperties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        uint32_t              m_MipLevels      = 1;
        uint32_t              m_ArrayLayers    = 1;
        VkSampleCountFlagBits m_Samples        = VK_SAMPLE_COUNT_1_BIT;

        // ImageView
        VkImageView    m_ImageView = VK_NULL_HANDLE;

        // Sampler (optional)
        VkSampler      m_Sampler = VK_NULL_HANDLE;
    };

}
