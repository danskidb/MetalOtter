#include "Otter/Core/Window.hpp"
#include "loguru.hpp"
#include <set>

namespace Otter 
{
	static void check_vk_result(VkResult err)
	{
		if (err == 0)
			return;

		LOG_F(ERROR, "[vulkan] Error: VkResult = %d\n", err);

		if (err < 0)
			abort();
	}

	Window::Window(glm::vec2 size, std::string title, VkInstance vulkanInstance)
	{
		this->vulkanInstance = vulkanInstance;
		this->title = title;
		
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		handle = glfwCreateWindow((int)size.x, (int)size.y, title.c_str(), nullptr, nullptr);
		LOG_F(INFO, "Initializing new window \'%s\' with size %gx%g", title.c_str(), size.x, size.y);
		
		InitializeVulkan();
		initialized = true;
	}

	Window::~Window()
	{
		if (!IsValid())
		{
			LOG_F(WARNING, "Window \'%s\' was not valid at destruction time.", title.c_str());
			return;
		}

		VkResult err = vkDeviceWaitIdle(logicalDevice);
		check_vk_result(err);

		vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
		vkDestroyDevice(logicalDevice, nullptr);
		glfwDestroyWindow(handle);
	}

	bool Window::ShouldBeDestroyed()
	{
		if (!IsValid())
			return true;
		else
			return glfwWindowShouldClose(handle) != 0;
	}

	void Window::OnTick()
	{
		if (!IsValid())
			return;
	}

	bool Window::InitializeVulkan()
	{
		CreateSurface();
		SelectPhysicalDevice();
		CreateLogicalDevice();
		return true;
	}

	void Window::CreateSurface()
	{
    	VkResult err = glfwCreateWindowSurface(vulkanInstance, handle, nullptr, &surface);
		check_vk_result(err);
	}

	void Window::SelectPhysicalDevice()
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
		LOG_F(INFO, "%s is using GPU %s", this->title.c_str(), properties.deviceName);
	}

	bool Window::IsPhysicalDeviceSuitable(VkPhysicalDevice gpu)
	{
		QueueFamilyIndices indices = FindQueueFamilies(gpu);
		return indices.graphicsFamily.has_value() && HasDeviceExtensionSupport(gpu); 
	}

	bool Window::HasDeviceExtensionSupport(VkPhysicalDevice gpu)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(gpu, nullptr, &extensionCount, availableExtensions.data());

		// Make a copy of the required extensions and remove them if they are present on the device.
		// If no extensions are part of the list anymore, we have them all and we're good to go.
		std::set<std::string> requiredExtensions(requiredPhysicalDeviceExtensions.begin(), requiredPhysicalDeviceExtensions.end());
		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty();
	}

	QueueFamilyIndices Window::FindQueueFamilies(VkPhysicalDevice gpu)
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

	void Window::CreateLogicalDevice()
	{
		// Specify queues to be created alongside the device.
		assert(physicalDevice != VK_NULL_HANDLE);
		QueueFamilyIndices indices = FindQueueFamilies(physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsFamily.value(), indices.presentFamily.value()};

		float queuePriority = 1.0f;
		for (uint32_t queueFamily : uniqueQueueFamilies) {
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			queueCreateInfo.queueCount = 1;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		// leaving everything to false until we want to do more special things with VK.
		VkPhysicalDeviceFeatures deviceFeatures{};

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
}