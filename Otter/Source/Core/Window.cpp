#include "Otter/Core/Window.hpp"
#include "loguru.hpp"
#include <vector>

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

    	VkResult err = glfwCreateWindowSurface(vulkanInstance, handle, nullptr, &surface);
		check_vk_result(err);

		initialized = true;
	}

	Window::~Window()
	{
		if (!IsValid())
			return;

		VkResult err = vkDeviceWaitIdle(vulkanDevice);
		check_vk_result(err);

		vkDestroyDescriptorPool(vulkanDevice, vulkanDescriptorPool, nullptr);
		vkDestroySurfaceKHR(vulkanInstance, surface, nullptr);
		vkDestroyDevice(vulkanDevice, nullptr);

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
		if (!IsValid() || !initialized)
			return;
	}

	bool Window::InitializeVulkan()
	{
		SelectPhysicalDevice();

		VkResult err;

		// Select graphics queue family
		{
			uint32_t count;
			vkGetPhysicalDeviceQueueFamilyProperties(vulkanPhysicalDevice, &count, NULL);
			VkQueueFamilyProperties* queues = (VkQueueFamilyProperties*)malloc(sizeof(VkQueueFamilyProperties) * count);
			vkGetPhysicalDeviceQueueFamilyProperties(vulkanPhysicalDevice, &count, queues);
			for (uint32_t i = 0; i < count; i++)
				if (queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					vulkanQueueFamily = i;
					break;
				}
			free(queues);
			assert(vulkanQueueFamily != (uint32_t)-1);
		}

		// Create Logical Device (with 1 queue)
		{
			int device_extension_count = 1;
			const char* device_extensions[] = { "VK_KHR_swapchain" };
			const float queue_priority[] = { 1.0f };
			VkDeviceQueueCreateInfo queue_info[1] = {};
			queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queue_info[0].queueFamilyIndex = vulkanQueueFamily;
			queue_info[0].queueCount = 1;
			queue_info[0].pQueuePriorities = queue_priority;
			VkDeviceCreateInfo create_info = {};
			create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			create_info.queueCreateInfoCount = sizeof(queue_info) / sizeof(queue_info[0]);
			create_info.pQueueCreateInfos = queue_info;
			create_info.enabledExtensionCount = device_extension_count;
			create_info.ppEnabledExtensionNames = device_extensions;
			err = vkCreateDevice(vulkanPhysicalDevice, &create_info, nullptr, &vulkanDevice);
			check_vk_result(err);
			vkGetDeviceQueue(vulkanDevice, vulkanQueueFamily, 0, &vulkanQueue);
		}
		if (err != VK_SUCCESS)
			return false;

		// Create Descriptor Pool
		{
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

			int arraysize = (int)(sizeof(pool_sizes) / sizeof(*(pool_sizes)));
			pool_info.maxSets = 1000 * arraysize;
			pool_info.poolSizeCount = (uint32_t)arraysize;
			pool_info.pPoolSizes = pool_sizes;
			err = vkCreateDescriptorPool(vulkanDevice, &pool_info, nullptr, &vulkanDescriptorPool);
			check_vk_result(err);
		}
		if (err != VK_SUCCESS)
			return false;

		return true;
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

		// If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available. This covers
		// most common cases (multi-gpu/integrated+dedicated graphics). Handling more complicated setups (multiple
		// dedicated GPUs) is out of scope of this sample.
		int use_gpu = 0;
		for (int i = 0; i < (int)gpuCount; i++)
		{
			VkPhysicalDeviceProperties properties;
			vkGetPhysicalDeviceProperties(gpus[i], &properties);
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
			{
				use_gpu = i;
				break;
			}
		}

		vulkanPhysicalDevice = gpus[use_gpu];

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(vulkanPhysicalDevice, &properties);
		LOG_F(INFO, "%s is using GPU %s", this->title.c_str(), properties.deviceName);
	}
}