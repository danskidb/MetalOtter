#include "Otter/Systems/Renderer.hpp"
#include "Otter/Utilities/ShaderUtilities.hpp"
#include "loguru.hpp"
#include "imgui.h"
#include "imgui_impl_sdl.h"
#include "SDL_vulkan.h"
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <glm/gtc/matrix_transform.hpp>

namespace Otter::Systems
{
	static void check_vk_result(VkResult err)
	{
		if (err == 0)
			return;

		LOG_F(ERROR, "[vulkan] Error: VkResult = %d\n", err);

		if (err < 0)
			abort();
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
	{
		LOG_F(ERROR, "[vulkan] validation layer: %s", pCallbackData->pMessage);
        return VK_FALSE;
    }

	void Renderer::OnStart()
	{
		currentFrame = 0;
		deltaTime = 0;

		if(!CreateVulkanInstance())
		{
			LOG_F(ERROR, "Failed to create vulkan instance");
			return;
		}

		CreateSurface();
		SelectPhysicalDevice();
		CreateLogicalDevice();

		// Memory Allocator
		VmaAllocatorCreateInfo allocatorCreateInfo{};
		allocatorCreateInfo.flags = 0;
		allocatorCreateInfo.physicalDevice = physicalDevice;
		allocatorCreateInfo.device = logicalDevice;
		allocatorCreateInfo.instance = vulkanInstance;
		vmaCreateAllocator(&allocatorCreateInfo, &allocator);

		// Load shader modules
		vert = ShaderUtilities::LoadShaderModule("Assets/Shaders/Simple/Simple.vert", logicalDevice);
		frag = ShaderUtilities::LoadShaderModule("Assets/Shaders/Simple/Simple.frag", logicalDevice);
		if(vert == VK_NULL_HANDLE || frag == VK_NULL_HANDLE)
			return;

		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateDescriptorSetLayout();
		CreateGraphicsPipeline();
		CreateFrameBuffers();
		CreateCommandPool();
		CreateTextureImage();
		CreateTextureSampler();
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		CreateCommandBuffer();
		CreateSyncObjects();

		if (imGuiAllowed)
			SetupImGui();

		initialized = true;
	}

	void Renderer::OnStop()
	{
		VkResult err = vkDeviceWaitIdle(logicalDevice);
		check_vk_result(err);

		if (imGuiAllowed)
		{
			ImGui_ImplVulkan_Shutdown();
			ImGui_ImplSDL2_Shutdown();
			ImGui::DestroyContext();
		}

		DestroySwapChain();

    	vkDestroySampler(logicalDevice, textureSampler, nullptr);
    	vkDestroyImageView(logicalDevice, textureImageView, nullptr);
		vmaDestroyImage(allocator, textureImage, textureImageMemory);
		vmaDestroyBuffer(allocator, vertexBuffer, vertexBufferAllocation);
		vmaDestroyBuffer(allocator, indexBuffer, indexBufferAllocation);
		for(size_t i = 0; i < uniformBufferAllocations.size(); i++)
			vmaDestroyBuffer(allocator, uniformBuffers[i], uniformBufferAllocations[i]);

		vmaDestroyAllocator(allocator);
		vkDestroyDescriptorSetLayout(logicalDevice, descriptorSetLayout, nullptr);
		vkDestroyShaderModule(logicalDevice, vert, nullptr);
    	vkDestroyShaderModule(logicalDevice, frag, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(logicalDevice, imageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(logicalDevice, renderFinishedSemaphores[i], nullptr);
			vkDestroyFence(logicalDevice, inFlightFences[i], nullptr);
		}
		vkDestroyDescriptorPool(logicalDevice, descriptorPool, nullptr);
		vkDestroyDescriptorPool(logicalDevice, imguiDescriptorPool, nullptr);
		vkDestroyCommandPool(logicalDevice, commandPool, nullptr);
		vkDestroyDevice(logicalDevice, nullptr);
		vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);

		vkDestroyInstance(vulkanInstance, nullptr);
		vulkanInstance = nullptr;
	}

	void Renderer::OnTick(float deltaTime)
	{
		if (!initialized)
			return;

		this->deltaTime = deltaTime;

		if (imGuiAllowed)
		{
			ImGui_ImplVulkan_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();

			if(onDrawImGui)
				onDrawImGui();

			ImGui::Render();
		}

		DrawFrame();
	}

	void Renderer::OnSDLEvent(SDL_Event* event)
	{
		if (!initialized)
			return;

		if (!imGuiAllowed)
			return;

		ImGui_ImplSDL2_ProcessEvent(event);
	}

	bool Renderer::CreateVulkanInstance()
	{
		//AppInfo
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = SDL_GetWindowTitle(handle);
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Otter";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		//CreateInfo
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

        auto extensions = GetRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(requiredValidationLayers.size());
            createInfo.ppEnabledLayerNames = requiredValidationLayers.data();

            PopulateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

		VkResult result = vkCreateInstance(&createInfo, nullptr, &vulkanInstance);
		check_vk_result(result);

		return result == VK_SUCCESS;
	}

	VkResult Renderer::CreateDebugUtilsMessengerEXT(VkInstance vulkanInstance, const VkDebugUtilsMessengerCreateInfoEXT* createInfo, const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vulkanInstance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(vulkanInstance, createInfo, allocator, debugMessenger);
		} else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void Renderer::DestroyDebugUtilsMessengerEXT(VkInstance vulkanInstance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* allocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vulkanInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(vulkanInstance, debugMessenger, allocator);
		}
	}

	bool Renderer::CheckValidationLayerSupport()
	{
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : requiredValidationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
	}

	std::vector<const char*> Renderer::GetRequiredExtensions()
	{
		//TODO move to Window so we have 2 vulkan instances and get the correct result
		unsigned int extensionCount = 0;
		SDL_Vulkan_GetInstanceExtensions(handle, &extensionCount, nullptr);
		std::vector<const char *> extensionNames(extensionCount);
		SDL_Vulkan_GetInstanceExtensions(handle, &extensionCount, extensionNames.data());

        return extensionNames;
	}

	void Renderer::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = vkDebugCallback;
	}

	void Renderer::SetupDebugMessenger()
	{
        if (!enableValidationLayers)
			return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        PopulateDebugMessengerCreateInfo(createInfo);

        VkResult result = CreateDebugUtilsMessengerEXT(vulkanInstance, &createInfo, nullptr, &debugMessenger);
		check_vk_result(result);
	}

	void Renderer::CreateSurface()
	{
		SDL_bool result = SDL_Vulkan_CreateSurface(handle, vulkanInstance, &surface);
		if(!result)
			LOG_F(ERROR, "Failed to create surface: %s", SDL_GetError());

		assert(result);
	}

	void Renderer::SelectPhysicalDevice()
	{
		uint32_t gpuCount;
		VkResult err = vkEnumeratePhysicalDevices(vulkanInstance, &gpuCount, NULL);
		check_vk_result(err);
		assert(gpuCount > 0);	//TODO make custom macro with std::runtime_error and Loguru.

		std::vector<VkPhysicalDevice> gpus(gpuCount);
		err = vkEnumeratePhysicalDevices(vulkanInstance, &gpuCount, gpus.data());
		check_vk_result(err);
		assert(err == VK_SUCCESS);

		std::vector<VkPhysicalDevice> suitableGpus{};
		for (const auto gpu : gpus)
			if (IsPhysicalDeviceSuitable(gpu))
				suitableGpus.push_back(gpu);
		assert(suitableGpus.size() > 0);

		// If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available. This covers
		// most common cases (multi-gpu/integrated+dedicated graphics). Handling more complicated setups (multiple
		// dedicated GPUs) is out of scope of this sample.
		int useGpu = 0;
		for (int i = 0; i < suitableGpus.size(); i++)
		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(suitableGpus[i], &properties);
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				useGpu = i;
				break;
			}
		}
		physicalDevice = suitableGpus[useGpu];
		assert(physicalDevice != VK_NULL_HANDLE);

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		LOG_F(INFO, "Using GPU %s", properties.deviceName);
	}

	bool Renderer::IsPhysicalDeviceSuitable(VkPhysicalDevice gpu)
	{
		QueueFamilyIndices indices = FindQueueFamilies(gpu);

		bool extensionsSupported = HasDeviceExtensionSupport(gpu);

		bool swapChainAdequate = false;
		if (extensionsSupported)
		{
			SwapChainSupportDetails swapChainSupport = FindSwapChainSupport(gpu);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
    	vkGetPhysicalDeviceFeatures(gpu, &supportedFeatures);

		return indices.graphicsFamily.has_value() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
	}

	bool Renderer::HasDeviceExtensionSupport(VkPhysicalDevice gpu)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, availableExtensions.data());

		// Make a copy of the required extensions and remove them if they are present on the device.
		// If no extensions are part of the list anymore, we have them all and we're good to go.
		std::set<std::string> requiredExtensions(requiredPhysicalDeviceExtensions.begin(), requiredPhysicalDeviceExtensions.end());
		for (const auto& extension : availableExtensions)
			requiredExtensions.erase(extension.extensionName);

		return requiredExtensions.empty();
	}

	QueueFamilyIndices Renderer::FindQueueFamilies(VkPhysicalDevice gpu)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queueFamilyCount, queueFamilies.data());

		// Find at least one queue family that supports Graphics.
		int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			VkBool32 presentSupport = false;
			vkGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &presentSupport);
			if (presentSupport)
				indices.presentFamily = i;

			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
				indices.graphicsFamily = i;

			i++;
		}

		return indices;
	}

	void Renderer::CreateLogicalDevice()
	{
		// Specify queues to be created alongside the device.
		assert(physicalDevice != VK_NULL_HANDLE);
		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		// Create the logical device.
		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;
		createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredPhysicalDeviceExtensions.size());
		createInfo.ppEnabledExtensionNames = requiredPhysicalDeviceExtensions.data();
		createInfo.enabledLayerCount = 0;	// Validation layers are disabled on application level currently.

		VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice);
		check_vk_result(result);

		// After creation, get a reference to our queues.
		vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
		vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
	}

	void Renderer::CreateSwapChain()
	{
		SwapChainSupportDetails swapChainSupport = FindSwapChainSupport(physicalDevice);
		VkSurfaceFormatKHR surfaceFormat = SelectSwapChainSurfaceFormat(swapChainSupport.formats);
		VkPresentModeKHR presentMode = SelectSwapChainPresentMode(swapChainSupport.presentModes);
		VkExtent2D extent = SelectSwapChainExtent(swapChainSupport.capabilities);

		uint32_t swapChainImageCount = swapChainSupport.capabilities.minImageCount + 1;
		uint32_t preClamp = swapChainImageCount;
		swapChainImageCount = std::clamp(swapChainImageCount, swapChainSupport.capabilities.minImageCount, swapChainSupport.capabilities.maxImageCount);
		if (swapChainImageCount != preClamp)
			LOG_F(WARNING, "Swap Chain Image Count preconfigured was out of supported bounds: old[%u] corrected[%u] min[%u] max[%u].", preClamp, swapChainImageCount, swapChainSupport.capabilities.minImageCount, swapChainSupport.capabilities.maxImageCount);

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = surface;
		createInfo.minImageCount = swapChainImageCount;
		createInfo.imageFormat = surfaceFormat.format;
		createInfo.imageColorSpace = surfaceFormat.colorSpace;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1;	// increase for stereoscopic 3d
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;	//direct usage, modify if you want to run post-processing etc. 

		// Specify how to handle swap chain images across multiple queue families (graphics, present)
		// There are two ways to handle images that are accessed from multiple queues:
    	// VK_SHARING_MODE_EXCLUSIVE
		// An image is owned by one queue family at a time and ownership must be explicitly transferred before using it in another queue family. This option offers the best performance.
    	//
		// VK_SHARING_MODE_CONCURRENT
		// Images can be used across multiple queue families without explicit ownership transfers.

		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);
		uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		// If the queue families differ, use concurrent mdoe so we don't have to deal with ownership.
		if (indices.graphicsFamily != indices.presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamilyIndices;
		}
		else
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}

		createInfo.preTransform = swapChainSupport.capabilities.currentTransform;	// We can specify that a certain transform should be applied to images in the swap chain if it is supported (supportedTransforms in capabilities), like a 90 degree clockwise rotation or horizontal flip.
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;	// Ignore alpha for blending with other windows.
		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE;	// We don't care about the color of pixels that are obscured, ie minimized or if another window is over.
		createInfo.oldSwapchain = VK_NULL_HANDLE;

		VkResult result = vkCreateSwapchainKHR(logicalDevice, &createInfo, nullptr, &swapChain);
		check_vk_result(result);

		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, nullptr);
		swapChainImages.resize(swapChainImageCount);
		vkGetSwapchainImagesKHR(logicalDevice, swapChain, &swapChainImageCount, swapChainImages.data());
		swapChainImageFormat = surfaceFormat.format;
		swapChainExtent = extent;

		quad.model = glm::rotate(glm::mat4(1.0f), 0*glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));
		quad.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		quad.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float) swapChainExtent.height, 0.1f, 10.0f);
		quad.proj[1][1] *= -1;
	}

	void Renderer::DestroySwapChain()
	{
		for (auto framebuffer : swapChainFramebuffers)
        	vkDestroyFramebuffer(logicalDevice, framebuffer, nullptr);

		for (auto imageView : swapChainImageViews)
			vkDestroyImageView(logicalDevice, imageView, nullptr);

		vkDestroyRenderPass(logicalDevice, renderPass, nullptr);
		vkDestroyPipeline(logicalDevice, graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(logicalDevice, pipelineLayout, nullptr);
		vkDestroySwapchainKHR(logicalDevice, swapChain, nullptr);
	}

	void Renderer::RecreateSwapChain()
	{
		//TODO high DPI support (SDL_WINDOW_ALLOW_HIGHDPI at window create)
		int width = 0, height = 0;
		SDL_GetWindowSize(handle, &width, &height);

		vkDeviceWaitIdle(logicalDevice);
		DestroySwapChain();

		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFrameBuffers();

		currentFrame = 0;
		framebufferResized = false;
		
		if(onFramebufferResized)
			onFramebufferResized({width, height});
	}

	SwapChainSupportDetails Renderer::FindSwapChainSupport(VkPhysicalDevice gpu)
	{
		SwapChainSupportDetails result;

		// Capabilities (min/max number of images in swap chain, min/max width and height of images)
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &result.capabilities);

		// Formats: Pixel format, colour space, etc.
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			result.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, result.formats.data());
		}

		// Present modes: conditions for "swapping" images to the screen
		uint32_t presentModeCount;
		vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, nullptr);
		if (presentModeCount != 0)
		{
			result.presentModes.resize(presentModeCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, result.presentModes.data());
		}

		return result;
	}

	VkSurfaceFormatKHR Renderer::SelectSwapChainSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		// Pick SRGB if we can have it, otherwise we'll just go with whatever is the first thing available.
		for (const auto& availableFormat : availableFormats)
			if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
				return availableFormat;

		return availableFormats[0];
	}

	VkPresentModeKHR Renderer::SelectSwapChainPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		/**
			VK_PRESENT_MODE_IMMEDIATE_KHR
			Images submitted by your application are transferred to the screen right away, which may result in tearing.
			
			VK_PRESENT_MODE_FIFO_KHR
			The swap chain is a queue where the display takes an image from the front of the queue when the display is refreshed and the program inserts rendered images at the back of the queue. 
			If the queue is full then the program has to wait. This is most similar to vertical sync as found in modern games.
			The moment that the display is refreshed is known as "vertical blank".
			***This is the only mode guaranteed to be available***
			
			VK_PRESENT_MODE_FIFO_RELAXED_KHR
			This mode only differs from the previous one if the application is late and the queue was empty at the last vertical blank. 
			Instead of waiting for the next vertical blank, the image is transferred right away when it finally arrives. This may result in visible tearing.
			
			VK_PRESENT_MODE_MAILBOX_KHR
			This is another variation of the second mode. 
			Instead of blocking the application when the queue is full, the images that are already queued are simply replaced with the newer ones. 
			This mode can be used to render frames as fast as possible while still avoiding tearing, resulting in fewer latency issues than standard vertical sync. 
			This is commonly known as "triple buffering", although the existence of three buffers alone does not necessarily mean that the framerate is unlocked.

			TODO: investigate device energy impact between modes.
		*/

		// Pick mailbox if its available, otherwise revert to FIFO/vsync.
		for (const auto& availablePresentMode : availablePresentModes)
			if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
				return availablePresentMode;

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D Renderer::SelectSwapChainExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		// With display scaling (osx retina display, windows scaling), the screen coords and pixel sizes may differ.
		// Find the actual extent in pixels, and return it.
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
			return capabilities.currentExtent;
		else
		{
			int width, height;
			//glfwGetFramebufferSize(handle, &width, &height);

			VkExtent2D actualExtent =
			{
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	VkImageView Renderer::CreateImageView(VkImage image, VkFormat format) {
		VkImageViewCreateInfo viewInfo{};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		viewInfo.image = image;
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		viewInfo.format = format;
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		VkImageView imageView;
		VkResult result = vkCreateImageView(logicalDevice, &viewInfo, nullptr, &imageView);
		check_vk_result(result);

		return imageView;
	}

	void Renderer::CreateImageViews()
	{
		swapChainImageViews.resize(swapChainImages.size());
		for (size_t i = 0; i < swapChainImages.size(); i++)
			swapChainImageViews[i] = CreateImageView(swapChainImages[i], swapChainImageFormat);
	}

	void Renderer::CreateRenderPass()
	{
	    VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;	
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;	

		VkResult result = vkCreateRenderPass(logicalDevice, &renderPassInfo, nullptr, &renderPass);
		check_vk_result(result);
	}

	void Renderer::CreateDescriptorSetLayout()
	{
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1;
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.binding = 1;
		samplerLayoutBinding.descriptorCount = 1;
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.pImmutableSamplers = nullptr;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = {uboLayoutBinding, samplerLayoutBinding};
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
		layoutInfo.pBindings = bindings.data();

		VkResult result = vkCreateDescriptorSetLayout(logicalDevice, &layoutInfo, nullptr, &descriptorSetLayout);
		check_vk_result(result);
	}

	void Renderer::CreateGraphicsPipeline()
	{
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vertShaderStageInfo.module = vert;
		vertShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = frag;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

		// Fixed functions of render pipeline
		auto bindingDescription = Vertex::getBindingDescription();
		auto attributeDescriptions = Vertex::getAttributeDescriptions();
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float) swapChainExtent.width;
		viewport.height = (float) swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissor{};
		scissor.offset = {0, 0};
		scissor.extent = swapChainExtent;

		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		// The rasterizer takes the geometry that is shaped by the vertices from the vertex shader and turns it into fragments to be colored by the fragment shader.
		// It also performs depth testing, face culling and the scissor test, and it can be configured to output fragments that fill entire polygons or just the edges (wireframe rendering).
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE;
		rasterizer.rasterizerDiscardEnable = VK_FALSE;
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
		rasterizer.lineWidth = 1.0f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		// AA method, currently disabled.
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;
		colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
		colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
		colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
		colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f; // Optional
		colorBlending.blendConstants[1] = 0.0f; // Optional
		colorBlending.blendConstants[2] = 0.0f; // Optional
		colorBlending.blendConstants[3] = 0.0f; // Optional

		// Dynamic States - A limited amount of the state that we've specified in the previous structs can actually be changed without recreating the pipeline
		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};

		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1; // Optional
		pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout; // Optional
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		VkResult result = vkCreatePipelineLayout(logicalDevice, &pipelineLayoutInfo, nullptr, &pipelineLayout);
		check_vk_result(result);

		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;	
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = nullptr; // Optional
		pipelineInfo.layout = pipelineLayout;
		pipelineInfo.renderPass = renderPass;
		pipelineInfo.subpass = 0;	
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		result = vkCreateGraphicsPipelines(logicalDevice, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
		check_vk_result(result);
	}

	void Renderer::CreateFrameBuffers()
	{
		swapChainFramebuffers.resize(swapChainImageViews.size());
		for (size_t i = 0; i < swapChainImageViews.size(); i++)
		{
			VkImageView attachments[] = { swapChainImageViews[i] };

			VkFramebufferCreateInfo framebufferInfo{};
			framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebufferInfo.renderPass = renderPass;
			framebufferInfo.attachmentCount = 1;
			framebufferInfo.pAttachments = attachments;
			framebufferInfo.width = swapChainExtent.width;
			framebufferInfo.height = swapChainExtent.height;
			framebufferInfo.layers = 1;

			VkResult result = vkCreateFramebuffer(logicalDevice, &framebufferInfo, nullptr, &swapChainFramebuffers[i]);
			check_vk_result(result);
		}
	}

	void Renderer::CreateVertexBuffer()
	{
		/*
			Load the Vertices into GPU memory by means of 
			RAM -> Staging Buffer -> Vertex Buffer.
			Reason we use the staging buffer is because the most optimal memory for the GPU is non-accessible by the CPU (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT).
			On integrated GPU's this can be ignored as all graphics data does not have to go through PCIe, but we're not handling that here at the moment.
			More info: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html
		*/
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

		// Staging Buffer
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		VmaAllocationInfo allocInfo = {};
		VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &stagingBuffer, &stagingBufferAllocation, &allocInfo);
		check_vk_result(result);

		memcpy(allocInfo.pMappedData, vertices.data(), (size_t)bufferInfo.size);

		// Vertex Buffer
		bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		result = vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &vertexBuffer, &vertexBufferAllocation, nullptr);
		check_vk_result(result);

		CopyBuffer(stagingBuffer, vertexBuffer, bufferSize);

		vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);
	}

	void Renderer::CreateIndexBuffer()
	{
		// See CreateVertexBuffer for some more info on how this works.

		VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;

		// Staging Buffer
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		VmaAllocationInfo allocInfo = {};
		VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &stagingBuffer, &stagingBufferAllocation, &allocInfo);
		check_vk_result(result);

		memcpy(allocInfo.pMappedData, indices.data(), (size_t)bufferInfo.size);

		// Index Buffer
		bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = bufferSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

		allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
		result = vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &indexBuffer, &indexBufferAllocation, nullptr);
		check_vk_result(result);

		CopyBuffer(stagingBuffer, indexBuffer, bufferSize);

		vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);
	}

	void Renderer::CreateUniformBuffers()
	{
		VkDeviceSize bufferSize = sizeof(UniformBufferObject);
		uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		uniformBufferAllocations.resize(MAX_FRAMES_IN_FLIGHT);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			bufferInfo.size = bufferSize;
			bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			VmaAllocationCreateInfo allocCreateInfo = {};
			allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;
			allocCreateInfo.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
			VmaAllocationInfo allocInfo = {};
			VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &uniformBuffers[i], &uniformBufferAllocations[i], &allocInfo);
			check_vk_result(result);
		}
	}

	void Renderer::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

		VkBufferCopy copyRegion{};
		copyRegion.srcOffset = 0; // Optional
		copyRegion.dstOffset = 0; // Optional
		copyRegion.size = size;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

		EndSingleTimeCommands(commandBuffer);
	}

	VkCommandBuffer Renderer::BeginSingleTimeCommands()
	{
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = commandPool;
		allocInfo.commandBufferCount = 1;

		VkCommandBuffer commandBuffer;
		VkResult result = vkAllocateCommandBuffers(logicalDevice, &allocInfo, &commandBuffer);
		check_vk_result(result);

		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		check_vk_result(result);

		return commandBuffer;
	}

	void Renderer::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		VkResult result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		check_vk_result(result);
		result = vkQueueWaitIdle(graphicsQueue);
		check_vk_result(result);

		vkFreeCommandBuffers(logicalDevice, commandPool, 1, &commandBuffer);
	}

	void Renderer::CreateDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkResult result = vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &descriptorPool);
		check_vk_result(result);

		// IMGUI
		if (!imGuiAllowed)
			return;

		VkDescriptorPoolSize pool_sizes[] =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        VkDescriptorPoolCreateInfo pool_info = {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
        pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
        pool_info.pPoolSizes = pool_sizes;
        result = vkCreateDescriptorPool(logicalDevice, &pool_info, VK_NULL_HANDLE, &imguiDescriptorPool);
        check_vk_result(result);
	}

	void Renderer::CreateDescriptorSets()
	{
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		allocInfo.pSetLayouts = layouts.data();

		descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		VkResult result = vkAllocateDescriptorSets(logicalDevice, &allocInfo, descriptorSets.data());
		check_vk_result(result);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorBufferInfo bufferInfo{};
			bufferInfo.buffer = uniformBuffers[i];
			bufferInfo.offset = 0;
			bufferInfo.range = sizeof(UniformBufferObject);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			imageInfo.imageView = textureImageView;
			imageInfo.sampler = textureSampler;

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0;
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &bufferInfo;

			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0;
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1;
			descriptorWrites[1].pImageInfo = &imageInfo;

			vkUpdateDescriptorSets(logicalDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
		}
	}

	void Renderer::CreateCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		VkResult result = vkCreateCommandPool(logicalDevice, &poolInfo, nullptr, &commandPool);
		check_vk_result(result);
	}

	void Renderer::CreateCommandBuffer()
	{
		commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = commandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

		VkResult result = vkAllocateCommandBuffers(logicalDevice, &allocInfo, commandBuffers.data());
		check_vk_result(result);
	}

	void Renderer::CreateTextureImage()
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load("Assets/Textures/floral_shoppe.jpg", &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		VkDeviceSize imageSize = texWidth * texHeight * 4;

		if (!pixels)
		{
			LOG_F(ERROR, "Failed to load texture image");
			abort();
		}

		// Move data to staging buffer
		VkBuffer stagingBuffer = VK_NULL_HANDLE;
		VmaAllocation stagingBufferAllocation = VK_NULL_HANDLE;
		VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufferInfo.size = imageSize;
		bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | VMA_ALLOCATION_CREATE_MAPPED_BIT;
		VmaAllocationInfo stagingAllocInfo = {};
		VkResult result = vmaCreateBuffer(allocator, &bufferInfo, &allocCreateInfo, &stagingBuffer, &stagingBufferAllocation, &stagingAllocInfo);
		check_vk_result(result);

		memcpy(stagingAllocInfo.pMappedData, pixels, (size_t)bufferInfo.size);
		stbi_image_free(pixels);

		// Then, to a vulkan image
		CreateImage(
			{ texWidth, texHeight }, 
			VK_FORMAT_R8G8B8A8_SRGB, 
			VK_IMAGE_TILING_OPTIMAL, 
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			textureImage,
			textureImageMemory
		);

		// Copy staging buffer to texture image
		TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(stagingBuffer, textureImage, {texWidth, texHeight});

		// Make the image accessible to shaders
		TransitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
		vmaDestroyBuffer(allocator, stagingBuffer, stagingBufferAllocation);

		textureImageView = CreateImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB);

	}

	void Renderer::CreateImage(Vec2D size, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImage& image, VmaAllocation& imageMemory)
	{
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D;
		imageInfo.extent.width = static_cast<uint32_t>(size.x);
		imageInfo.extent.height = static_cast<uint32_t>(size.y);
		imageInfo.extent.depth = 1;
		imageInfo.mipLevels = 1;
		imageInfo.arrayLayers = 1;
		imageInfo.format = VK_FORMAT_R8G8B8A8_SRGB;
		imageInfo.tiling = tiling;
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		imageInfo.usage = usage;
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

		VmaAllocationCreateInfo imageAllocCreateInfo = {};
		imageAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		VmaAllocationInfo imageAllocInfo = {};
		VkResult result = vmaCreateImage(allocator, &imageInfo, &imageAllocCreateInfo, &image, &imageMemory, &imageAllocInfo);
		check_vk_result(result);
	}

	void Renderer::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
		VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = 1;

		// https://vulkan-tutorial.com/Texture_mapping/Images#page_Transition-barrier-masks
		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		} 
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
		else
		{
			LOG_F(ERROR, "Unsupported layout transition!");
			abort();
		}

		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, destinationStage,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		EndSingleTimeCommands(commandBuffer);
	}

	void Renderer::CopyBufferToImage(VkBuffer buffer, VkImage image, Vec2D size)
	{
    	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

		VkBufferImageCopy region{};
		region.bufferOffset = 0;
		region.bufferRowLength = 0;
		region.bufferImageHeight = 0;

		region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel = 0;
		region.imageSubresource.baseArrayLayer = 0;
		region.imageSubresource.layerCount = 1;

		region.imageOffset = {0, 0, 0};
		region.imageExtent =
		{
			static_cast<uint32_t>(size.x),
			static_cast<uint32_t>(size.y),
			1
		};	

		vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
		);


    	EndSingleTimeCommands(commandBuffer);
	}

	void Renderer::CreateTextureSampler()
	{
		// More info here: https://vulkan-tutorial.com/Texture_mapping/Image_view_and_sampler#page_Samplers

		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.anisotropyEnable = VK_TRUE;

		// TODO: pull this out and make this static data.
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(physicalDevice, &properties);
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;

		samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
		samplerInfo.unnormalizedCoordinates = VK_FALSE;
		samplerInfo.compareEnable = VK_FALSE;
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		VkResult result = vkCreateSampler(logicalDevice, &samplerInfo, nullptr, &textureSampler);
		check_vk_result(result);
	}

	void Renderer::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // Optional
		beginInfo.pInheritanceInfo = nullptr; // Optional

		VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
		check_vk_result(result);

		VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = renderPass;
		renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
		renderPassInfo.renderArea.offset = {0, 0};
		renderPassInfo.renderArea.extent = swapChainExtent;
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);

		VkBuffer vertexBuffers[] = {vertexBuffer};
		VkDeviceSize offsets[] = {0};
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSets[currentFrame], 0, nullptr);
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
		if(imGuiAllowed)
			ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), commandBuffer);

        vkCmdEndRenderPass(commandBuffer);
        result = vkEndCommandBuffer(commandBuffer);
		check_vk_result(result);
	}

	void Renderer::CreateSyncObjects()
	{
		imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
    	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

		VkResult result;
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			result = vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
			check_vk_result(result);
			result = vkCreateSemaphore(logicalDevice, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);
			check_vk_result(result);
			result = vkCreateFence(logicalDevice, &fenceInfo, nullptr, &inFlightFences[i]);
			check_vk_result(result);
		}
	}

	void Renderer::DrawFrame()
	{
    	vkWaitForFences(logicalDevice, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
    	VkResult result = vkAcquireNextImageKHR(logicalDevice, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
		{
			RecreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS)
			check_vk_result(result);
		
		vkResetFences(logicalDevice, 1, &inFlightFences[currentFrame]);
		vkResetCommandBuffer(commandBuffers[currentFrame], 0);
		RecordCommandBuffer(commandBuffers[currentFrame], imageIndex);
		UpdateUniformBuffer(currentFrame);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

		VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
		VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.pWaitDstStageMask = waitStages;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

		VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
		submitInfo.signalSemaphoreCount = 1;
		submitInfo.pSignalSemaphores = signalSemaphores;

		result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
		check_vk_result(result);

		VkPresentInfoKHR presentInfo{};
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pWaitSemaphores = signalSemaphores;

		VkSwapchainKHR swapChains[] = {swapChain};
		presentInfo.swapchainCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // Optional

		result = vkQueuePresentKHR(presentQueue, &presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized)
		{
			RecreateSwapChain();
			return;
		}
		else if (result != VK_SUCCESS)
			check_vk_result(result);

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
	}

	void Renderer::UpdateUniformBuffer(uint32_t currentImage)
	{
		/*
			Using a UBO this way is not the most efficient way to pass frequently changing values to the shader.
			A more efficient way to pass a small buffer of data to shaders are push constants. We may look at these in a future chapter.
		*/
		quad.model = glm::rotate(quad.model, deltaTime*glm::radians(90.f), glm::vec3(0.0f, 0.0f, 1.0f));

		void* data;
		vmaMapMemory(allocator, uniformBufferAllocations[currentImage], &data);
		memcpy(data, &quad, sizeof(quad));
		vmaUnmapMemory(allocator, uniformBufferAllocations[currentImage]);
	}

	void Renderer::SetupImGui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking

		ImGui::StyleColorsDark();
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

		int w, h;
		//glfwGetFramebufferSize(handle, &w, &h);

		// Setup Platform/Renderer backends
		ImGui_ImplSDL2_InitForVulkan(handle);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = vulkanInstance;
		init_info.PhysicalDevice = physicalDevice;
		init_info.Device = logicalDevice;
		init_info.Queue = graphicsQueue;
		init_info.DescriptorPool = imguiDescriptorPool;
		init_info.Subpass = 0;
		init_info.MinImageCount = minImageCount;
		init_info.ImageCount = imageCount;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.CheckVkResultFn = check_vk_result;
		ImGui_ImplVulkan_Init(&init_info, renderPass);

		// upload fonts
        VkResult err = vkResetCommandPool(logicalDevice, commandPool, 0);
        check_vk_result(err);
        VkCommandBufferBeginInfo begin_info = {};
        begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        begin_info.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        err = vkBeginCommandBuffer(commandBuffers[currentFrame], &begin_info);
        check_vk_result(err);

        ImGui_ImplVulkan_CreateFontsTexture(commandBuffers[currentFrame]);

        VkSubmitInfo end_info = {};
        end_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        end_info.commandBufferCount = 1;
        end_info.pCommandBuffers = &commandBuffers[currentFrame];
        err = vkEndCommandBuffer(commandBuffers[currentFrame]);
        check_vk_result(err);
        err = vkQueueSubmit(graphicsQueue, 1, &end_info, VK_NULL_HANDLE);
        check_vk_result(err);

        err = vkDeviceWaitIdle(logicalDevice);
        check_vk_result(err);
        ImGui_ImplVulkan_DestroyFontUploadObjects();
	}
}