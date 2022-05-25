#pragma once

#include "glm/vec2.hpp"
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"
#include <string>

namespace Otter
{
	class Window 
	{
	public:
		Window(glm::vec2 size, std::string title, VkInstance vulkanInstance);
		virtual ~Window();

		inline bool IsValid() { return handle != nullptr; }
		bool ShouldBeDestroyed();

		virtual void OnTick();

	private:
		bool initialized = false;
		std::string title;

		GLFWwindow* handle;
		VkInstance vulkanInstance = VK_NULL_HANDLE;
		VkSurfaceKHR surface = VK_NULL_HANDLE;

		VkPhysicalDevice vulkanPhysicalDevice = VK_NULL_HANDLE;
		VkDevice vulkanDevice = VK_NULL_HANDLE;

		uint32_t vulkanQueueFamily = (uint32_t)-1;
		VkQueue vulkanQueue = VK_NULL_HANDLE;
		VkDescriptorPool vulkanDescriptorPool = VK_NULL_HANDLE;

		bool InitializeVulkan();
		void SelectPhysicalDevice();	// Picks a GPU to run Vulkan on.
		//TODO; Queue families
	};
}