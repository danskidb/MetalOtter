#pragma once

#include "glm/vec2.hpp"
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"
#include <string>
#include <optional>
#include <vector>

namespace Otter
{
	static const std::vector<const char*> requiredPhysicalDeviceExtensions= {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME
	};

	struct QueueFamilyIndices
	{
		//std::optional can query var.has_value() to see if its been modified i.e. if the queue family exists. This because 0 can be a valid queue family index.
		std::optional<uint32_t> graphicsFamily;
		std::optional<uint32_t> presentFamily;

		inline bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
	};

	class Window 
	{
	public:
		Window(glm::vec2 size, std::string title, VkInstance vulkanInstance);
		virtual ~Window();

		inline bool IsValid() { return handle != nullptr && initialized; }
		bool ShouldBeDestroyed();

		virtual void OnTick();

	private:
		bool initialized = false;
		std::string title;

		GLFWwindow* handle;
		VkInstance vulkanInstance = VK_NULL_HANDLE;
		VkSurfaceKHR surface = VK_NULL_HANDLE;

		VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
		VkDevice logicalDevice = VK_NULL_HANDLE;

		VkQueue graphicsQueue = VK_NULL_HANDLE;
		VkQueue presentQueue = VK_NULL_HANDLE;

		bool InitializeVulkan();
		void CreateSurface();

		void SelectPhysicalDevice();	// Picks a GPU to run Vulkan on.
		bool IsPhysicalDeviceSuitable(VkPhysicalDevice gpu);
		bool HasDeviceExtensionSupport(VkPhysicalDevice gpu);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice gpu);		// Everything in VK works with queues, we want to query what the GPU can do.

		void CreateLogicalDevice();
	};
}