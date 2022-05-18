#include "Otter/Core/Window.hpp"
#include "loguru.hpp"

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
		vulkanInstanceRef = vulkanInstance;
		this->title = title;
		
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		handle = glfwCreateWindow((int)size.x, (int)size.y, title.c_str(), nullptr, nullptr);
		LOG_F(INFO, "Initializing new window \'%s\' with size %gx%g", title.c_str(), size.x, size.y);
		
		CreateVulkanDevice();

    	VkResult err = glfwCreateWindowSurface(vulkanInstance, handle, nullptr, &surface);
		check_vk_result(err);

		// Create framebuffers
		int w, h;
		glfwGetFramebufferSize(handle, &w, &h);
		ImGui_ImplVulkanH_Window* wd = &mainWindowData;
		SetupVulkanWindow(wd, surface, w, h);

		// Setup IMGUI context
		imGuiContext = ImGui::CreateContext();
		ImGui::SetCurrentContext(imGuiContext);
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		ImGui::StyleColorsDark();

		ImGui_ImplGlfw_InitForVulkan(handle, true);
		ImGui_ImplVulkan_InitInfo init_info = {};
		init_info.Instance = vulkanInstance;
		init_info.PhysicalDevice = vulkanPhysicalDevice;
		init_info.Device = vulkanDevice;
		init_info.QueueFamily = vulkanQueueFamily;
		init_info.Queue = vulkanQueue;
		init_info.PipelineCache = vulkanPipelineCache;
		init_info.DescriptorPool = vulkanDescriptorPool;
		init_info.Subpass = 0;
		init_info.MinImageCount = minImageCount;
		init_info.ImageCount = wd->ImageCount;
		init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		init_info.Allocator = nullptr;
		init_info.CheckVkResultFn = check_vk_result;
		ImGui_ImplVulkan_Init(&init_info, wd->RenderPass);

		initialized = true;
	}

	Window::~Window()
	{
		if (!IsValid())
			return;
		
		VkResult err = vkDeviceWaitIdle(vulkanDevice);
		check_vk_result(err);

		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();

    	ImGui_ImplVulkanH_DestroyWindow(vulkanInstanceRef, vulkanDevice, &mainWindowData, nullptr);
		vkDestroyDevice(vulkanDevice, nullptr);

		glfwDestroyWindow(handle);
	}

    bool show_demo_window = true;
	void Window::OnTick()
	{
		if (!IsValid() || !initialized)
			return;

		ImGui::SetCurrentContext(imGuiContext);

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

		// ImGui::ShowDemoWindow(&show_demo_window);

		// ImGui::Render();
        // ImDrawData* draw_data = ImGui::GetDrawData();
        // const bool is_minimized = (draw_data->DisplaySize.x <= 0.0f || draw_data->DisplaySize.y <= 0.0f);
        // if (!is_minimized)
        // {
		// 	ImGui_ImplVulkanH_Window* wd = &mainWindowData;
        //     FrameRender(wd, draw_data);
        //     FramePresent(wd);
        // }
	}

	bool Window::ShouldBeDestroyed()
	{
		if (!IsValid())
			return true;
		else
			return glfwWindowShouldClose(handle) != 0;
	}

	bool Window::CreateVulkanDevice()
	{
		VkResult err;

		// Select GPU
		{
			uint32_t gpu_count;
			err = vkEnumeratePhysicalDevices(vulkanInstanceRef, &gpu_count, NULL);
			check_vk_result(err);
			assert(gpu_count > 0);

			VkPhysicalDevice* gpus = (VkPhysicalDevice*)malloc(sizeof(VkPhysicalDevice) * gpu_count);
			err = vkEnumeratePhysicalDevices(vulkanInstanceRef, &gpu_count, gpus);
			check_vk_result(err);
			assert(err == VK_SUCCESS);

			// If a number >1 of GPUs got reported, find discrete GPU if present, or use first one available. This covers
			// most common cases (multi-gpu/integrated+dedicated graphics). Handling more complicated setups (multiple
			// dedicated GPUs) is out of scope of this sample.
			int use_gpu = 0;
			for (int i = 0; i < (int)gpu_count; i++)
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

			free(gpus);
		}

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

		// Create Descriptor Pool
		{
			VkDescriptorPoolSize pool_sizes[] =
			{
				{ VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
				// { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
				// { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
				// { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
				// { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
				// { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
				// { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
				// { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
				// { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
				// { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
				// { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
			};
			VkDescriptorPoolCreateInfo pool_info = {};
			pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			pool_info.maxSets = 1000 * sizeof(pool_sizes);
			pool_info.poolSizeCount = (uint32_t)sizeof(pool_sizes);
			pool_info.pPoolSizes = pool_sizes;
			err = vkCreateDescriptorPool(vulkanDevice, &pool_info, nullptr, &vulkanDescriptorPool);
			check_vk_result(err);
		}

		return true;
	}

	void Window::SetupVulkanWindow(ImGui_ImplVulkanH_Window* wd, VkSurfaceKHR surface, int width, int height)
	{
		wd->Surface = surface;

		// Check for WSI support
		VkBool32 res;
		vkGetPhysicalDeviceSurfaceSupportKHR(vulkanPhysicalDevice, vulkanQueueFamily, wd->Surface, &res);
		if (res != VK_TRUE)
		{
			LOG_F(ERROR, "Error no WSI support on physical device 0");
			exit(-1);
		}

		// Select Surface Format
		const VkFormat requestSurfaceImageFormat[] = { VK_FORMAT_B8G8R8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_B8G8R8_UNORM, VK_FORMAT_R8G8B8_UNORM };
		const VkColorSpaceKHR requestSurfaceColorSpace = VK_COLORSPACE_SRGB_NONLINEAR_KHR;
		wd->SurfaceFormat = ImGui_ImplVulkanH_SelectSurfaceFormat(vulkanPhysicalDevice, wd->Surface, requestSurfaceImageFormat, (size_t)IM_ARRAYSIZE(requestSurfaceImageFormat), requestSurfaceColorSpace);

		// Select Present Mode
	#ifdef IMGUI_UNLIMITED_FRAME_RATE
		VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_MAILBOX_KHR, VK_PRESENT_MODE_IMMEDIATE_KHR, VK_PRESENT_MODE_FIFO_KHR };
	#else
		VkPresentModeKHR present_modes[] = { VK_PRESENT_MODE_FIFO_KHR };
	#endif
		wd->PresentMode = ImGui_ImplVulkanH_SelectPresentMode(vulkanPhysicalDevice, wd->Surface, &present_modes[0], IM_ARRAYSIZE(present_modes));
		LOG_F(INFO, "[vulkan] Selected PresentMode = %d", wd->PresentMode);

		// Create SwapChain, RenderPass, Framebuffer, etc.
		assert(minImageCount >= 2);
		ImGui_ImplVulkanH_CreateOrResizeWindow(vulkanInstanceRef, vulkanPhysicalDevice, vulkanDevice, wd, vulkanQueueFamily, nullptr, width, height, minImageCount);
	}

	void Window::FrameRender(ImGui_ImplVulkanH_Window* wd, ImDrawData* drawData)
	{

	}

	void Window::FramePresent(ImGui_ImplVulkanH_Window* wd)
	{

	}
}