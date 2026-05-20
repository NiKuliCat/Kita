#include "kita_pch.h"
#include "VulkanContext.h"
#include "core/Log.h"
#include "platform/windows/WindowsWindow.h"
namespace Kita {

    namespace {

        constexpr std::array<const char*, 1> s_ValidationLayers = { "VK_LAYER_KHRONOS_validation" };

        constexpr std::array<const char*, 1> s_DeviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

        static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugCallback(
            VkDebugUtilsMessageSeverityFlagBitsEXT severity,
            VkDebugUtilsMessageTypeFlagsEXT type,
            const VkDebugUtilsMessengerCallbackDataEXT* callbackData,
            void* userData)
        {
            (void)type;
            (void)userData;

            if (severity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
                KITA_CORE_WARN("Vulkan validation: {0}", callbackData->pMessage);
            else
                KITA_CORE_TRACE("Vulkan validation: {0}", callbackData->pMessage);

            return VK_FALSE;
        }

        void VKCheck(VkResult result, const char* message)
        {
            if (result != VK_SUCCESS)
            {
                KITA_CORE_ERROR("{0}, VkResult = {1}", message, static_cast<int32_t>(result));
                throw std::runtime_error(message);
            }
        }

        void PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
        {
            createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity =
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType =
                VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = VulkanDebugCallback;
        }

        VkResult CreateDebugUtilsMessengerEXT(
            VkInstance instance,
            const VkDebugUtilsMessengerCreateInfoEXT* createInfo,
            const VkAllocationCallbacks* allocator,
            VkDebugUtilsMessengerEXT* messenger)
        {
            auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));

            if (func)
                return func(instance, createInfo, allocator, messenger);

            return VK_ERROR_EXTENSION_NOT_PRESENT;
        }

        void DestroyDebugUtilsMessengerEXT(
            VkInstance instance,
            VkDebugUtilsMessengerEXT messenger,
            const VkAllocationCallbacks* allocator)
        {
            auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
                vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

            if (func)
                func(instance, messenger, allocator);
        }

    }

    VulkanContext::VulkanContext(GLFWwindow* windowHandle, VulkanConfig config)
        :m_WindowHandle(windowHandle),m_Config(config)
    {
        KITA_CORE_ASSERT(m_WindowHandle, "VulkanContext window is null!");
    }

    VulkanContext::~VulkanContext()
    {
        Shutdown();
    }

    void VulkanContext::Init()
    {
        CreateInstance();
        SetupDebugMessenger();
        CreateSurface();
        PickPhysicalDevice();
        CreateLogicalDevice();
        CreateSwapchain();
        CreateSwapchainImageViews();
        CreateCommandPool();
        CreateCommandBuffers();
        CreateSyncObjects();

        KITA_CORE_INFO("VulkanContext initialized");
    }

    void VulkanContext::Shutdown()
    {
        if (m_Instance == VK_NULL_HANDLE)
            return;

        WaitIdle();

        for (size_t i = 0; i < m_ImageAvailableSemaphores.size(); ++i)
        {
            vkDestroySemaphore(m_Device, m_ImageAvailableSemaphores[i], nullptr);
            vkDestroyFence(m_Device, m_InFlightFences[i], nullptr);
        }

        for (VkSemaphore semaphore : m_RenderFinishedSemaphores)
            vkDestroySemaphore(m_Device, semaphore, nullptr);

        m_ImageAvailableSemaphores.clear();
        m_RenderFinishedSemaphores.clear();
        m_InFlightFences.clear();
        m_ImagesInFlight.clear();

        if (m_CommandPool != VK_NULL_HANDLE)
        {
            vkDestroyCommandPool(m_Device, m_CommandPool, nullptr);
            m_CommandPool = VK_NULL_HANDLE;
        }

        CleanupSwapchain();

        if (m_Device != VK_NULL_HANDLE)
        {
            vkDestroyDevice(m_Device, nullptr);
            m_Device = VK_NULL_HANDLE;
        }

        if (m_Surface != VK_NULL_HANDLE)
        {
            vkDestroySurfaceKHR(m_Instance, m_Surface, nullptr);
            m_Surface = VK_NULL_HANDLE;
        }

        if (m_DebugMessenger != VK_NULL_HANDLE)
        {
            DestroyDebugUtilsMessengerEXT(m_Instance, m_DebugMessenger, nullptr);
            m_DebugMessenger = VK_NULL_HANDLE;
        }

        vkDestroyInstance(m_Instance, nullptr);
        m_Instance = VK_NULL_HANDLE;
    }

    bool VulkanContext::BeginFrame()
    {
        KITA_CORE_ASSERT(!m_FrameStarted, "BeginFrame called while frame is already started");

        vkWaitForFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame], VK_TRUE, UINT64_MAX);

        VkResult result = vkAcquireNextImageKHR(
            m_Device,
            m_Swapchain,
            UINT64_MAX,
            m_ImageAvailableSemaphores[m_CurrentFrame],
            VK_NULL_HANDLE,
            &m_CurrentImageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR)
        {
            RecreateSwapchain();
            return false;
        }

        VKCheck(result, "Failed to acquire swapchain image");

        if (m_ImagesInFlight[m_CurrentImageIndex] != VK_NULL_HANDLE)
            vkWaitForFences(m_Device, 1, &m_ImagesInFlight[m_CurrentImageIndex], VK_TRUE, UINT64_MAX);

        m_ImagesInFlight[m_CurrentImageIndex] = m_InFlightFences[m_CurrentFrame];

        vkResetFences(m_Device, 1, &m_InFlightFences[m_CurrentFrame]);

        VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
        vkResetCommandBuffer(commandBuffer, 0);
        BeginCommandBuffer(commandBuffer);

        m_FrameStarted = true;
        return true;
    }

    void VulkanContext::EndFrame()
    {
        KITA_CORE_ASSERT(m_FrameStarted, "EndFrame called without BeginFrame");

        VkCommandBuffer commandBuffer = m_CommandBuffers[m_CurrentFrame];
        EndCommandBuffer(commandBuffer);

        VkSemaphore waitSemaphores[] = { m_ImageAvailableSemaphores[m_CurrentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

        VkSemaphore renderFinishedSemaphore = m_RenderFinishedSemaphores[m_CurrentImageIndex];
        VkSemaphore signalSemaphores[] = { renderFinishedSemaphore };

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        VKCheck(
            vkQueueSubmit(m_GraphicsQueue, 1, &submitInfo, m_InFlightFences[m_CurrentFrame]),
            "Failed to submit Vulkan command buffer");

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = &m_Swapchain;
        presentInfo.pImageIndices = &m_CurrentImageIndex;

        VkResult result = vkQueuePresentKHR(m_PresentQueue, &presentInfo);

        m_FrameStarted = false;

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_FramebufferResized)
        {
            m_FramebufferResized = false;
            RecreateSwapchain();
        }
        else
        {
            VKCheck(result, "Failed to present Vulkan swapchain image");
        }

        m_CurrentFrame = (m_CurrentFrame + 1) % m_Config.FramesInFlight;
    }

    void VulkanContext::OnResize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0)
            return;

        m_FramebufferResized = true;
    }

    void VulkanContext::WaitIdle()
    {
        if (m_Device != VK_NULL_HANDLE)
            vkDeviceWaitIdle(m_Device);
    }


    void VulkanContext::CreateInstance()
    {
        if (m_Config.EnableValidation && !CheckValidationLayerSupport())
        {
            KITA_CORE_WARN("Vulkan validation layers requested but unavailable");
            m_Config.EnableValidation = false;
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Kita";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "Kita Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        auto extensions = GetRequiredInstanceExtensions();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        if (m_Config.EnableValidation)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayers.size());
            createInfo.ppEnabledLayerNames = s_ValidationLayers.data();

            PopulateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = &debugCreateInfo;
        }
        VKCheck(vkCreateInstance(&createInfo, nullptr, &m_Instance), "Failed to create Vulkan instance");
    }

    void VulkanContext::SetupDebugMessenger()
    {
        if (!m_Config.EnableValidation)
            return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        PopulateDebugMessengerCreateInfo(createInfo);

        VKCheck(CreateDebugUtilsMessengerEXT(m_Instance, &createInfo, nullptr, &m_DebugMessenger), "Failed to create Vulkan debug messenger");
    }

    void VulkanContext::CreateSurface()
    {
        VKCheck(glfwCreateWindowSurface(m_Instance, m_WindowHandle, nullptr, &m_Surface), "Failed to create Vulkan window surface");
    }


    void VulkanContext::PickPhysicalDevice()
    {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, nullptr);

        KITA_CORE_ASSERT(deviceCount > 0, "No Vulkan physical device found");

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(m_Instance, &deviceCount, devices.data());

        for (VkPhysicalDevice device : devices)
        {
            if (IsDeviceSuitable(device))
            {
                m_PhysicalDevice = device;
                break;
            }
        }

        KITA_CORE_ASSERT(m_PhysicalDevice != VK_NULL_HANDLE, "No suitable Vulkan physical device found");

        VkPhysicalDeviceProperties properties{};
        vkGetPhysicalDeviceProperties(m_PhysicalDevice, &properties);
        KITA_CORE_INFO("Vulkan GPU: {0}", properties.deviceName);
    }


    void VulkanContext::CreateLogicalDevice()
    {
        QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

        m_GraphicsQueueFamilyIndex = indices.GraphicsFamily.value();
        m_PresentQueueFamilyIndex = indices.PresentFamily.value();

        std::set<uint32_t> uniqueQueueFamilies = {
            indices.GraphicsFamily.value(),
            indices.PresentFamily.value()
        };

        float queuePriority = 1.0f;
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

        for (uint32_t queueFamily : uniqueQueueFamilies)
        {
            VkDeviceQueueCreateInfo queueCreateInfo{};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures2 deviceFeatures2{};
        deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;

        VkPhysicalDeviceVulkan11Features vulkan11Features{};
        vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;

        VkPhysicalDeviceVulkan13Features vulkan13Features{};
        vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
        deviceFeatures2.pNext = &vulkan11Features;
        vulkan11Features.pNext = &vulkan13Features;

        vkGetPhysicalDeviceFeatures2(m_PhysicalDevice, &deviceFeatures2);

        KITA_CORE_ASSERT(
            vulkan11Features.shaderDrawParameters == VK_TRUE,
            "Vulkan physical device does not support shaderDrawParameters");
        KITA_CORE_ASSERT(
            vulkan13Features.dynamicRendering == VK_TRUE,
            "Vulkan physical device does not support dynamicRendering");

        vulkan11Features.shaderDrawParameters = VK_TRUE;
        vulkan13Features.dynamicRendering = VK_TRUE;

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.pNext = &vulkan11Features;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        createInfo.pEnabledFeatures = nullptr;
        createInfo.enabledExtensionCount = static_cast<uint32_t>(s_DeviceExtensions.size());
        createInfo.ppEnabledExtensionNames = s_DeviceExtensions.data();

        if (m_Config.EnableValidation)
        {
            createInfo.enabledLayerCount = static_cast<uint32_t>(s_ValidationLayers.size());
            createInfo.ppEnabledLayerNames = s_ValidationLayers.data();
        }

        VKCheck(vkCreateDevice(m_PhysicalDevice, &createInfo, nullptr, &m_Device), "Failed to create Vulkan logical device");

        vkGetDeviceQueue(m_Device, indices.GraphicsFamily.value(), 0, &m_GraphicsQueue);
        vkGetDeviceQueue(m_Device, indices.PresentFamily.value(), 0, &m_PresentQueue);
    }


    void VulkanContext::CreateSwapchain()
    {
        SwapchainSupportDetails support = QuerySwapchainSupport(m_PhysicalDevice);

        VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(support.Formats);
        VkPresentModeKHR presentMode = ChooseSwapPresentMode(support.PresentModes);
        VkExtent2D extent = ChooseSwapExtent(support.Capabilities);

        uint32_t imageCount = support.Capabilities.minImageCount + 1;
        if (support.Capabilities.maxImageCount > 0 && imageCount > support.Capabilities.maxImageCount)
            imageCount = support.Capabilities.maxImageCount;

        QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);
        uint32_t queueFamilyIndices[] = {
            indices.GraphicsFamily.value(),
            indices.PresentFamily.value()
        };

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_Surface;
        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        if (indices.GraphicsFamily != indices.PresentFamily)
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        }
        else
        {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = support.Capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;
        createInfo.oldSwapchain = VK_NULL_HANDLE;

        VKCheck(vkCreateSwapchainKHR(m_Device, &createInfo, nullptr, &m_Swapchain), "Failed to create Vulkan swapchain");

        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, nullptr);
        m_SwapchainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_Device, m_Swapchain, &imageCount, m_SwapchainImages.data());
        m_SwapchainImageLayouts.assign(imageCount, VK_IMAGE_LAYOUT_UNDEFINED);
        m_ImagesInFlight.assign(imageCount, VK_NULL_HANDLE);

        m_SwapchainImageFormat = surfaceFormat.format;
        m_SwapchainExtent = extent;
    }


    void VulkanContext::CreateSwapchainImageViews()
    {
        m_SwapchainImageViews.resize(m_SwapchainImages.size());

        for (size_t i = 0; i < m_SwapchainImages.size(); ++i)
        {
            VkImageViewCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = m_SwapchainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = m_SwapchainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            VKCheck(
                vkCreateImageView(m_Device, &createInfo, nullptr, &m_SwapchainImageViews[i]),
                "Failed to create Vulkan swapchain image view");
        }
    }

    void VulkanContext::CreateCommandPool()
    {
        QueueFamilyIndices indices = FindQueueFamilies(m_PhysicalDevice);

        VkCommandPoolCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        createInfo.queueFamilyIndex = indices.GraphicsFamily.value();

        VKCheck(vkCreateCommandPool(m_Device, &createInfo, nullptr, &m_CommandPool), "Failed to create Vulkan command pool");
    }


    void VulkanContext::CreateCommandBuffers()
    {
        m_CommandBuffers.resize(m_Config.FramesInFlight);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_CommandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_CommandBuffers.size());

        VKCheck(vkAllocateCommandBuffers(m_Device, &allocInfo, m_CommandBuffers.data()), "Failed to allocate Vulkan command buffers");
    }


    void VulkanContext::CreateSyncObjects()
    {
        m_ImageAvailableSemaphores.resize(m_Config.FramesInFlight);
        m_RenderFinishedSemaphores.resize(m_SwapchainImages.size());
        m_InFlightFences.resize(m_Config.FramesInFlight);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (uint32_t i = 0; i < m_Config.FramesInFlight; ++i)
        {
            VKCheck(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_ImageAvailableSemaphores[i]), "Failed to create image available semaphore");
            VKCheck(vkCreateFence(m_Device, &fenceInfo, nullptr, &m_InFlightFences[i]), "Failed to create in-flight fence");
        }

        for (size_t i = 0; i < m_RenderFinishedSemaphores.size(); ++i)
            VKCheck(vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]), "Failed to create render finished semaphore");
    }

    void VulkanContext::CleanupSwapchain()
    {
        for (VkImageView imageView : m_SwapchainImageViews)
            vkDestroyImageView(m_Device, imageView, nullptr);

        m_SwapchainImageViews.clear();
        m_SwapchainImages.clear();
        m_SwapchainImageLayouts.clear();
        m_ImagesInFlight.clear();

        if (m_Swapchain != VK_NULL_HANDLE)
        {
            vkDestroySwapchainKHR(m_Device, m_Swapchain, nullptr);
            m_Swapchain = VK_NULL_HANDLE;
        }
    }


    void VulkanContext::RecreateSwapchain()
    {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(m_WindowHandle, &width, &height);

        while (width == 0 || height == 0)
        {
            glfwGetFramebufferSize(m_WindowHandle, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_Device);

        CleanupSwapchain();
        CreateSwapchain();
        CreateSwapchainImageViews();

        for (VkSemaphore semaphore : m_RenderFinishedSemaphores)
            vkDestroySemaphore(m_Device, semaphore, nullptr);
        m_RenderFinishedSemaphores.clear();
        m_RenderFinishedSemaphores.resize(m_SwapchainImages.size());
        m_ImagesInFlight.assign(m_SwapchainImages.size(), VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for (size_t i = 0; i < m_RenderFinishedSemaphores.size(); ++i)
        {
            VKCheck(
                vkCreateSemaphore(m_Device, &semaphoreInfo, nullptr, &m_RenderFinishedSemaphores[i]),
                "Failed to recreate render finished semaphore");
        }
    }

    bool VulkanContext::CheckValidationLayerSupport() const
    {
        uint32_t layerCount = 0;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : s_ValidationLayers)
        {
            bool found = false;

            for (const auto& layer : availableLayers)
            {
                if (strcmp(layerName, layer.layerName) == 0)
                {
                    found = true;
                    break;
                }
            }

            if (!found)
                return false;
        }

        return true;
    }

    std::vector<const char*> VulkanContext::GetRequiredInstanceExtensions() const
    {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (m_Config.EnableValidation)
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return extensions;
    }

    bool VulkanContext::IsDeviceSuitable(VkPhysicalDevice device) const
    {
        QueueFamilyIndices indices = FindQueueFamilies(device);
        bool extensionsSupported = CheckDeviceExtensionSupport(device);

        bool swapchainAdequate = false;
        if (extensionsSupported)
        {
            SwapchainSupportDetails support = QuerySwapchainSupport(device);
            swapchainAdequate = !support.Formats.empty() && !support.PresentModes.empty();
        }

        return indices.IsComplete() && extensionsSupported && swapchainAdequate;
    }

    bool VulkanContext::CheckDeviceExtensionSupport(VkPhysicalDevice device) const
    {
        uint32_t extensionCount = 0;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(s_DeviceExtensions.begin(), s_DeviceExtensions.end());

        for (const auto& extension : availableExtensions)
            requiredExtensions.erase(extension.extensionName);

        return requiredExtensions.empty();
    }


    VulkanContext::QueueFamilyIndices VulkanContext::FindQueueFamilies(VkPhysicalDevice device) const
    {
        QueueFamilyIndices indices{};

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        for (uint32_t i = 0; i < queueFamilyCount; ++i)
        {
            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
                indices.GraphicsFamily = i;

            VkBool32 presentSupport = VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_Surface, &presentSupport);

            if (presentSupport)
                indices.PresentFamily = i;

            if (indices.IsComplete())
                break;
        }

        return indices;
    }


    VulkanContext::SwapchainSupportDetails VulkanContext::QuerySwapchainSupport(VkPhysicalDevice device) const
    {
        SwapchainSupportDetails details{};

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_Surface, &details.Capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, nullptr);

        if (formatCount > 0)
        {
            details.Formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_Surface, &formatCount, details.Formats.data());
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, nullptr);

        if (presentModeCount > 0)
        {
            details.PresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_Surface, &presentModeCount, details.PresentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR VulkanContext::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const
    {
        for (const auto& format : formats)
        {
            if (format.format == VK_FORMAT_B8G8R8A8_UNORM &&
                format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
            {
                return format;
            }
        }

        return formats[0];
    }


    VkPresentModeKHR VulkanContext::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes) const
    {
        for (const auto& mode : presentModes)
        {
            if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
                return mode;
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanContext::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
    {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
            return capabilities.currentExtent;

        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(m_WindowHandle, &width, &height);

        VkExtent2D actualExtent{
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };

        actualExtent.width = std::clamp(
            actualExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);

        actualExtent.height = std::clamp(
            actualExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);

        return actualExtent;
    }


    void VulkanContext::BeginCommandBuffer(VkCommandBuffer commandBuffer)
    {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;

        VKCheck(vkBeginCommandBuffer(commandBuffer, &beginInfo), "Failed to begin Vulkan command buffer");
    }

    void VulkanContext::EndCommandBuffer(VkCommandBuffer commandBuffer)
    {
        VKCheck(vkEndCommandBuffer(commandBuffer), "Failed to end Vulkan command buffer");
    }

}
