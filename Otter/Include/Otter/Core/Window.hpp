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

		virtual void OnTick();

		bool ShouldBeDestroyed();
		bool CreateVulkanDevice();

	private:
		bool initialized = false;
		VkInstance vulkanInstanceRef;

		std::string title;
		GLFWwindow* handle;
		VkSurfaceKHR surface;

		VkPhysicalDevice vulkanPhysicalDevice;
		VkDevice vulkanDevice;
		uint32_t vulkanQueueFamily = (uint32_t)-1;
		VkQueue vulkanQueue;
		VkPipelineCache vulkanPipelineCache;
		VkDescriptorPool vulkanDescriptorPool;
		const int minImageCount = 2;
		bool swapChainRebuild = false;
	};
}