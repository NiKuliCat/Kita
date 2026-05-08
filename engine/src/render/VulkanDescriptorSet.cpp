#include "kita_pch.h"
#include "VulkanDescriptorSet.h"

#include "VulkanContext.h"
#include "VulkanUniformBuffer.h"
#include "VulkanStorageBuffer.h"
#include "core/Log.h"

namespace Kita {

    namespace
    {
        VKAPI_ATTR void VKCheck(VkResult result, const char* message)
        {
            if (result != VK_SUCCESS)
            {
                KITA_CORE_ERROR("{0}, VkResult = {1}", message, static_cast<int32_t>(result));
                throw std::runtime_error(message);
            }
        }
    }

    // ========================================================================
    // Lifecycle
    // ========================================================================

    VulkanDescriptorSet::VulkanDescriptorSet(VulkanContext& context, const CreateInfo& createInfo)
    {
        Init(context, createInfo);
    }

    VulkanDescriptorSet::~VulkanDescriptorSet()
    {
        Destroy();
    }

    VulkanDescriptorSet::VulkanDescriptorSet(VulkanDescriptorSet&& other) noexcept
        : m_Context(other.m_Context)
        , m_Name(std::move(other.m_Name))
        , m_Layout(other.m_Layout)
        , m_Pool(other.m_Pool)
        , m_Set(other.m_Set)
        , m_Info(std::move(other.m_Info))
    {
        other.m_Context = nullptr;
        other.m_Layout  = VK_NULL_HANDLE;
        other.m_Pool    = VK_NULL_HANDLE;
        other.m_Set     = VK_NULL_HANDLE;
    }

    VulkanDescriptorSet& VulkanDescriptorSet::operator=(VulkanDescriptorSet&& other) noexcept
    {
        if (this == &other)
            return *this;

        Destroy();

        m_Context = other.m_Context;
        m_Name    = std::move(other.m_Name);
        m_Layout  = other.m_Layout;
        m_Pool    = other.m_Pool;
        m_Set     = other.m_Set;
        m_Info    = std::move(other.m_Info);

        other.m_Context = nullptr;
        other.m_Layout  = VK_NULL_HANDLE;
        other.m_Pool    = VK_NULL_HANDLE;
        other.m_Set     = VK_NULL_HANDLE;

        return *this;
    }

    void VulkanDescriptorSet::Init(VulkanContext& context, const CreateInfo& createInfo)
    {
        KITA_CORE_ASSERT(context.GetDevice() != VK_NULL_HANDLE, "VulkanDescriptorSet: context has no device");
        KITA_CORE_ASSERT(!createInfo.Bindings.empty(), "VulkanDescriptorSet: bindings must not be empty");

        m_Context = &context;
        m_Name    = createInfo.Name.empty() ? "DescriptorSet" : createInfo.Name;
        m_Info    = createInfo;

        CreateLayout();
        CreatePool();
        AllocateSet();

        KITA_CORE_INFO("Created VulkanDescriptorSet '{0}' with {1} bindings", m_Name, m_Info.Bindings.size());
    }

    void VulkanDescriptorSet::Destroy()
    {
        if (!m_Context)
            return;

        VkDevice device = m_Context->GetDevice();
        if (device == VK_NULL_HANDLE)
        {
            m_Context = nullptr;
            return;
        }

        if (m_Pool != VK_NULL_HANDLE)
        {
            // Descriptor sets allocated from the pool are implicitly freed
            vkDestroyDescriptorPool(device, m_Pool, nullptr);
            m_Pool = VK_NULL_HANDLE;
            m_Set  = VK_NULL_HANDLE;
        }

        if (m_Layout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device, m_Layout, nullptr);
            m_Layout = VK_NULL_HANDLE;
        }

        m_Info.Bindings.clear();
        m_Context = nullptr;
    }

    // ========================================================================
    // Internal creation
    // ========================================================================

    void VulkanDescriptorSet::CreateLayout()
    {
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        layoutBindings.reserve(m_Info.Bindings.size());

        for (const auto& b : m_Info.Bindings)
        {
            VkDescriptorSetLayoutBinding layoutBinding{};
            layoutBinding.binding            = b.binding;
            layoutBinding.descriptorType     = b.type;
            layoutBinding.descriptorCount    = b.count;
            layoutBinding.stageFlags         = b.stageFlags;
            layoutBinding.pImmutableSamplers = nullptr;
            layoutBindings.push_back(layoutBinding);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
        layoutInfo.pBindings    = layoutBindings.data();

        VKCheck(
            vkCreateDescriptorSetLayout(m_Context->GetDevice(), &layoutInfo, nullptr, &m_Layout),
            "Failed to create descriptor set layout");
    }

    void VulkanDescriptorSet::CreatePool()
    {
        // Aggregate descriptor counts per type
        std::unordered_map<VkDescriptorType, uint32_t> typeCounts;
        for (const auto& b : m_Info.Bindings)
            typeCounts[b.type] += b.count;

        std::vector<VkDescriptorPoolSize> poolSizes;
        poolSizes.reserve(typeCounts.size());
        for (const auto& [type, count] : typeCounts)
        {
            VkDescriptorPoolSize poolSize{};
            poolSize.type            = type;
            poolSize.descriptorCount = count;
            poolSizes.push_back(poolSize);
        }

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.maxSets       = 1;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes    = poolSizes.data();

        VKCheck(
            vkCreateDescriptorPool(m_Context->GetDevice(), &poolInfo, nullptr, &m_Pool),
            "Failed to create descriptor pool");
    }

    void VulkanDescriptorSet::AllocateSet()
    {
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool     = m_Pool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts        = &m_Layout;

        VKCheck(
            vkAllocateDescriptorSets(m_Context->GetDevice(), &allocInfo, &m_Set),
            "Failed to allocate descriptor set");
    }

    // ========================================================================
    // Write bindings
    // ========================================================================

    void VulkanDescriptorSet::WriteUniformBuffer(uint32_t binding, VkBuffer buffer,
                                                  VkDeviceSize offset, VkDeviceSize range)
    {
        KITA_CORE_ASSERT(m_Set != VK_NULL_HANDLE, "VulkanDescriptorSet: not initialized");
        KITA_CORE_ASSERT(buffer != VK_NULL_HANDLE, "VulkanDescriptorSet: WriteUniformBuffer with null buffer");

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffer;
        bufferInfo.offset = offset;
        bufferInfo.range  = range;

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = m_Set;
        write.dstBinding      = binding;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        write.pBufferInfo     = &bufferInfo;

        vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &write, 0, nullptr);
    }

    void VulkanDescriptorSet::WriteUniformBuffer(uint32_t binding, const VulkanUniformBuffer& ubo)
    {
        VkDescriptorBufferInfo info = ubo.GetDescriptorInfo();
        WriteUniformBuffer(binding, info.buffer, info.offset, info.range);
    }

    void VulkanDescriptorSet::WriteStorageBuffer(uint32_t binding, VkBuffer buffer,
                                                  VkDeviceSize offset, VkDeviceSize range)
    {
        KITA_CORE_ASSERT(m_Set != VK_NULL_HANDLE, "VulkanDescriptorSet: not initialized");
        KITA_CORE_ASSERT(buffer != VK_NULL_HANDLE, "VulkanDescriptorSet: WriteStorageBuffer with null buffer");

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = buffer;
        bufferInfo.offset = offset;
        bufferInfo.range  = range;

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = m_Set;
        write.dstBinding      = binding;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        write.pBufferInfo     = &bufferInfo;

        vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &write, 0, nullptr);
    }
    void VulkanDescriptorSet::WriteStorageBuffer(uint32_t binding, const VulkanStorageBuffer& ssbo)
    {
        VkDescriptorBufferInfo info = ssbo.GetDescriptorInfo();
        WriteStorageBuffer(binding, info.buffer, info.offset, info.range);
    }

    void VulkanDescriptorSet::WriteImageSampler(uint32_t binding, VkImageView view, VkSampler sampler,
                                                 VkImageLayout layout)
    {
        KITA_CORE_ASSERT(m_Set != VK_NULL_HANDLE, "VulkanDescriptorSet: not initialized");
        KITA_CORE_ASSERT(view != VK_NULL_HANDLE, "VulkanDescriptorSet: WriteImageSampler with null view");

        VkDescriptorImageInfo imageInfo{};
        imageInfo.sampler     = sampler;
        imageInfo.imageView   = view;
        imageInfo.imageLayout = layout;

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = m_Set;
        write.dstBinding      = binding;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo      = &imageInfo;

        vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &write, 0, nullptr);
    }

    void VulkanDescriptorSet::WriteImageSampler(uint32_t binding, const VkDescriptorImageInfo& info)
    {
        KITA_CORE_ASSERT(m_Set != VK_NULL_HANDLE, "VulkanDescriptorSet: not initialized");

        VkWriteDescriptorSet write{};
        write.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet          = m_Set;
        write.dstBinding      = binding;
        write.dstArrayElement = 0;
        write.descriptorCount = 1;
        write.descriptorType  = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo      = &info;

        vkUpdateDescriptorSets(m_Context->GetDevice(), 1, &write, 0, nullptr);
    }

    // ========================================================================
    // Bind
    // ========================================================================

    void VulkanDescriptorSet::Bind(VkCommandBuffer cmd, VkPipelineLayout layout,
                                    uint32_t firstSet,
                                    const std::vector<uint32_t>& dynamicOffsets) const
    {
        KITA_CORE_ASSERT(cmd != VK_NULL_HANDLE, "VulkanDescriptorSet::Bind: command buffer is null");
        KITA_CORE_ASSERT(layout != VK_NULL_HANDLE, "VulkanDescriptorSet::Bind: pipeline layout is null");
        KITA_CORE_ASSERT(m_Set != VK_NULL_HANDLE, "VulkanDescriptorSet::Bind: descriptor set is null");

        vkCmdBindDescriptorSets(
            cmd,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            layout,
            firstSet,
            1,
            &m_Set,
            static_cast<uint32_t>(dynamicOffsets.size()),
            dynamicOffsets.empty() ? nullptr : dynamicOffsets.data());
    }

}
