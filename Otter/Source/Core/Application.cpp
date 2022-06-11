#include "Otter/Core/Application.hpp"
#include <chrono>
#include <iostream>
#include <loguru.hpp>
#include "SDL.h"
#include "SDL_vulkan.h"
#include "glslang/Include/glslang_c_interface.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace Otter {

	// static void glfw_error_callback(int error, const char* description)
	// {
	// 	LOG_F(ERROR, "Glfw Error %d: %s", error, description);
	// }

	static void check_vk_result(VkResult err)
	{
		if (err == 0)
			return;

		LOG_F(ERROR, "[vulkan] Error: VkResult = %d\n", err);

		if (err < 0)
			abort();
	}

	static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
		LOG_F(ERROR, "[vulkan] validation layer: %s", pCallbackData->pMessage);
        return VK_FALSE;
    }

	Application::Application()
	{
		appName = "OtterApplication";
		vulkanInstance = VK_NULL_HANDLE;
	}

	Application::~Application()
	{
	}

	void Application::Run(int argc, char* argv[], char* envp[])
	{
		loguru::init(argc, argv);
		loguru::add_file("otter.log", loguru::FileMode::Truncate, loguru::Verbosity_MAX);

		SDL_Init(SDL_INIT_EVERYTHING);

		if(!CreateVulkanInstance())
		{
			LOG_F(ERROR, "Failed to create vulkan instance");
			return;
		}

		glslang_initialize_process();

		OnStart();

		std::vector<std::shared_ptr<Otter::Window>> windowsToBeDestroyed;
		float dt = 0.0f;
		while (shouldTick)
		{
			auto startTime = std::chrono::high_resolution_clock::now();

			SDL_Event event;
			while (SDL_PollEvent(&event))
			{
				if(event.type == SDL_QUIT)
					for(auto window : windows)
						windowsToBeDestroyed.push_back(window);
				

				//...
			}
			OnTick(dt);

			for(auto window : windows)
			{
				if (window->ShouldBeDestroyed())
				{
					windowsToBeDestroyed.push_back(window);
					continue;
				}

				window->OnTick(dt);
			}
			
			for(auto window : windowsToBeDestroyed)
				DestroyWindow(window);
			windowsToBeDestroyed.clear();

			if (windows.empty())	// this causes the app to quit when the last window is closed.
				shouldTick = false;

			auto stopTime = std::chrono::high_resolution_clock::now();
			dt = std::chrono::duration<float, std::chrono::seconds::period>(stopTime - startTime).count();
		}

		windows.clear();

		OnStop();
		glslang_finalize_process();
		vkDestroyInstance(vulkanInstance, nullptr);
		vulkanInstance = nullptr;
		SDL_Quit();
	}

	bool Application::DestroyWindow(std::shared_ptr<Otter::Window> window)
	{
		auto it = windows.begin();
		bool found = false;

		while(it != windows.end())
			if(*it == window)
			{
				it = windows.erase(it);
				found = true;
			}
			else
				++it;

		return found;
	}

	bool Application::CreateVulkanInstance()
	{
		//AppInfo
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = this->appName.c_str();
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

	VkResult Application::CreateDebugUtilsMessengerEXT(VkInstance vulkanInstance, const VkDebugUtilsMessengerCreateInfoEXT* createInfo, const VkAllocationCallbacks* allocator, VkDebugUtilsMessengerEXT* debugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vulkanInstance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(vulkanInstance, createInfo, allocator, debugMessenger);
		} else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void Application::DestroyDebugUtilsMessengerEXT(VkInstance vulkanInstance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* allocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(vulkanInstance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(vulkanInstance, debugMessenger, allocator);
		}
	}

	bool Application::CheckValidationLayerSupport()
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

	std::vector<const char*> Application::GetRequiredExtensions()
	{
		//TODO move to Window so we have 2 vulkan instances and get the correct result
		unsigned int extensionCount = 0;
		SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, nullptr);
		std::vector<const char *> extensionNames(extensionCount);
		SDL_Vulkan_GetInstanceExtensions(nullptr, &extensionCount, extensionNames.data());

        return extensionNames;
	}

	void Application::PopulateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
	{
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = vkDebugCallback;
	}

	void Application::SetupDebugMessenger()
	{
        if (!enableValidationLayers)
			return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        PopulateDebugMessengerCreateInfo(createInfo);

        VkResult result = CreateDebugUtilsMessengerEXT(vulkanInstance, &createInfo, nullptr, &debugMessenger);
		check_vk_result(result);
	}
}