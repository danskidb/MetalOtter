#pragma once
#include "Otter/Core/System.hpp"
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.hpp"
#include "glm/vec2.hpp"
#include <optional>

namespace Otter::Systems
{
	static const int MAX_FRAMES_IN_FLIGHT = 2;
	static const std::vector<const char*> requiredPhysicalDeviceExtensions =
	{
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	struct QueueFamilyIndices
	{
		//std::optional can query var.has_value() to see if its been modified i.e. if the queue family exists. This because 0 can be a valid queue family index.
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		inline bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
	};

	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};	

	class Event;
	class Renderer : public System
	{
	public:
		virtual void OnStart();
		virtual void OnStop();
		virtual void OnTick(float deltaTime);

		inline void InvalidateFramebuffer() { framebufferResized = true; }
		inline void SetWindowHandle(GLFWwindow* windowHandle) { this->handle = windowHandle; }
		inline void SetVulkanInstance(VkInstance vulkanInstance) { this->vulkanInstance = vulkanInstance; }
		inline void SetFrameBufferResizedCallback(std::function<void(glm::vec2)> onFramebufferResized) { this->onFramebufferResized = onFramebufferResized; }

	private:
		bool initialized = false;
		std::function<void(glm::vec2)> onFramebufferResized;

		GLFWwindow* handle = nullptr;
		VkInstance vulkanInstance = VK_NULL_HANDLE;
		VkSurfaceKHR surface = VK_NULL_HANDLE;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice logicalDevice = VK_NULL_HANDLE;

		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;

		VkShaderModule vert = VK_NULL_HANDLE;
		VkShaderModule frag = VK_NULL_HANDLE;

		VkSwapchainKHR swapChain;
		std::vector<VkImage> swapChainImages;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		std::vector<VkImageView> swapChainImageViews;

		VkPipelineLayout pipelineLayout;
		VkRenderPass renderPass;
		VkPipeline graphicsPipeline;
		
		bool framebufferResized;
		std::vector<VkFramebuffer> swapChainFramebuffers;
		VkCommandPool commandPool;
		std::vector<VkCommandBuffer> commandBuffers;

		std::vector<VkSemaphore> imageAvailableSemaphores;
		std::vector<VkSemaphore> renderFinishedSemaphores;
		std::vector<VkFence> inFlightFences;
		uint32_t currentFrame = 0;

		void CreateSurface();

		void SelectPhysicalDevice();	// Picks a GPU to run Vulkan on.
		bool IsPhysicalDeviceSuitable(VkPhysicalDevice gpu);
		bool HasDeviceExtensionSupport(VkPhysicalDevice gpu);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice gpu);		// Everything in VK works with queues, we want to query what the GPU can do.

		void CreateLogicalDevice();

		void CreateSwapChain();
		void DestroySwapChain();	// Also destroys things reliant on the swap chain, like the framebuffer.
		void RecreateSwapChain();
		SwapChainSupportDetails FindSwapChainSupport(VkPhysicalDevice gpu);
		VkSurfaceFormatKHR SelectSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR SelectSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D SelectSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities); // Return the canvas size in actual pixels, not in screen coordinates.
		void CreateImageViews();

		void CreateRenderPass();
		void CreateGraphicsPipeline();
		void CreateFrameBuffers();

		void CreateCommandPool();
		void CreateCommandBuffer();
		void RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

		void CreateSyncObjects();
		void DrawFrame();
	};
}