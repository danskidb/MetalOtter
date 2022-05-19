#pragma once

#include "glm/vec2.hpp"
#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
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
		virtual void DrawImGui() = 0;

		bool ShouldBeDestroyed();
		bool CreateVulkanDevice();
		void SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height);
		void FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* drawData);
		void FramePresent(ImGui_ImplVulkanH_Window* wd);

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

		ImGuiContext* imGuiContext;
		ImGui_ImplVulkanH_Window mainWindowData;
		const int minImageCount = 2;
		bool swapChainRebuild = false;
	};
}