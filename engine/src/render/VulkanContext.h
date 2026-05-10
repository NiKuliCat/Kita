#pragma once
#include <vulkan/vulkan.h>
struct GLFWwindow;
namespace Kita {

	struct VulkanConfig
	{
		bool EnableValidation = true;
		uint32_t FramesInFlight = 2;
	};

	class VulkanContext
	{

	public:
		VulkanContext(GLFWwindow* windowHandle, VulkanConfig config = {});
		~VulkanContext();

		void Init();
		void Shutdown();

		void OnResize(uint32_t width, uint32_t height);

		bool BeginFrame();
		void EndFrame();

		void WaitIdle();

		VkInstance GetInstance() const { return m_Instance; }
		VkPhysicalDevice GetPhysicalDevice() const { return m_PhysicalDevice; }
		VkDevice GetDevice() const { return m_Device; }
		VkSurfaceKHR GetSurface() const { return m_Surface; }

		VkQueue GetGraphicsQueue() const { return m_GraphicsQueue; }
		VkQueue GetPresentQueue() const { return m_PresentQueue; }

		VkSwapchainKHR GetSwapchain() const { return m_Swapchain; }
		VkFormat GetSwapchainImageFormat() const { return m_SwapchainImageFormat; }
		VkExtent2D GetSwapchainExtent() const { return m_SwapchainExtent; }
		VkImageLayout GetSwapchainImageLayout(uint32_t imageIndex) const { return m_SwapchainImageLayouts.at(imageIndex); }
		void SetSwapchainImageLayout(uint32_t imageIndex, VkImageLayout layout) { m_SwapchainImageLayouts.at(imageIndex) = layout; }

		VkCommandPool GetCommandPool() const { return m_CommandPool; }
		VkCommandBuffer GetCurrentCommandBuffer() const { return m_CommandBuffers[m_CurrentFrame]; }

		uint32_t GetCurrentFrameIndex() const { return m_CurrentFrame; }
		uint32_t GetCurrentImageIndex() const { return m_CurrentImageIndex; }
		uint32_t GetFramesInFlight() const { return m_Config.FramesInFlight; }

		const std::vector<VkImage>& GetSwapchainImages() const { return m_SwapchainImages; }
		const std::vector<VkImageView>& GetSwapchainImageViews() const { return m_SwapchainImageViews; }

		uint32_t GetGraphicsQueueFamilyIndex() const { return m_GraphicsQueueFamilyIndex; }
	private:
		struct QueueFamilyIndices
		{
			std::optional<uint32_t> GraphicsFamily;
			std::optional<uint32_t> PresentFamily;

			bool IsComplete() const
			{
				return GraphicsFamily.has_value() && PresentFamily.has_value();
			}
		};

		struct SwapchainSupportDetails
		{
			VkSurfaceCapabilitiesKHR Capabilities{};
			std::vector<VkSurfaceFormatKHR> Formats;
			std::vector<VkPresentModeKHR> PresentModes;
		};
	private:
		void CreateInstance();
		void SetupDebugMessenger();
		void CreateSurface();
		void PickPhysicalDevice();
		void CreateLogicalDevice();
		void CreateSwapchain();
		void CreateSwapchainImageViews();
		void CreateCommandPool();
		void CreateCommandBuffers();
		void CreateSyncObjects();

		void CleanupSwapchain();
		void RecreateSwapchain();

		bool CheckValidationLayerSupport() const;
		std::vector<const char*> GetRequiredInstanceExtensions() const;

		bool IsDeviceSuitable(VkPhysicalDevice device) const;
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device) const;
		SwapchainSupportDetails QuerySwapchainSupport(VkPhysicalDevice device) const;

		VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats) const;
		VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& presentModes) const;
		VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

		void BeginCommandBuffer(VkCommandBuffer commandBuffer);
		void EndCommandBuffer(VkCommandBuffer commandBuffer);

	

	private:
		GLFWwindow* m_WindowHandle = nullptr;
		VulkanConfig m_Config{};

		VkInstance m_Instance = VK_NULL_HANDLE;
		VkDebugUtilsMessengerEXT m_DebugMessenger = VK_NULL_HANDLE;
		VkSurfaceKHR m_Surface = VK_NULL_HANDLE;

		VkPhysicalDevice m_PhysicalDevice = VK_NULL_HANDLE;
		VkDevice m_Device = VK_NULL_HANDLE;

		uint32_t m_GraphicsQueueFamilyIndex = 0;
		uint32_t m_PresentQueueFamilyIndex = 0;

		VkQueue m_GraphicsQueue = VK_NULL_HANDLE;
		VkQueue m_PresentQueue = VK_NULL_HANDLE;

		VkSwapchainKHR m_Swapchain = VK_NULL_HANDLE;
		std::vector<VkImage> m_SwapchainImages;
		std::vector<VkImageView> m_SwapchainImageViews;
		std::vector<VkImageLayout> m_SwapchainImageLayouts;
		VkFormat m_SwapchainImageFormat = VK_FORMAT_UNDEFINED;
		VkExtent2D m_SwapchainExtent{};

		VkCommandPool m_CommandPool = VK_NULL_HANDLE;
		std::vector<VkCommandBuffer> m_CommandBuffers;

		std::vector<VkSemaphore> m_ImageAvailableSemaphores;
		std::vector<VkSemaphore> m_RenderFinishedSemaphores;
		std::vector<VkFence> m_InFlightFences;
		std::vector<VkFence> m_ImagesInFlight;

		uint32_t m_CurrentFrame = 0;
		uint32_t m_CurrentImageIndex = 0;

		bool m_FramebufferResized = false;
		bool m_FrameStarted = false;
	};
}
