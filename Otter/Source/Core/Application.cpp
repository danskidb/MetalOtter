#include "Otter/Core/Application.hpp"
#include <chrono>
#include <iostream>
#include <GLFW/glfw3.h>
#include <loguru.hpp>

namespace Otter {

	static void glfw_error_callback(int error, const char* description)
	{
		LOG_F(ERROR, "Glfw Error %d: %s", error, description);
	}

	static void check_vk_result(VkResult err)
	{
		if (err == 0)
			return;

		LOG_F(ERROR, "[vulkan] Error: VkResult = %d\n", err);

		if (err < 0)
			abort();
	}

	Application::Application()
	{
		appName = "OtterApplication";
		vulkanInstance = VK_NULL_HANDLE;
	}

	Application::~Application()
	{
		windows.clear();
	}

	void Application::Run(int argc, char* argv[], char* envp[])
	{
		loguru::init(argc, argv);
		loguru::add_file("otter.log", loguru::FileMode::Truncate, loguru::Verbosity_MAX);

		glfwSetErrorCallback(glfw_error_callback);
		if(!glfwInit())
		{
			LOG_F(ERROR, "Failed to initialize GLFW");
			return;
		}

		if(!glfwVulkanSupported())
		{
			LOG_F(ERROR, "GLFW: Vulkan is not supported");
			return;
		}

		if(!CreateVulkanInstance())
		{
			LOG_F(ERROR, "Failed to create vulkan instance");
			return;
		}

		OnStart();

		float dt = 0.0f;
		while (shouldTick)
		{
			auto startTime = std::chrono::high_resolution_clock::now();

			OnTick(dt);
			for(auto window : windows)
			{
				if (window->ShouldBeDestroyed())
				{
					DestroyWindow(window);
					continue;
				}

				window->OnTick();
			}
			
			glfwPollEvents();
			if (windows.empty())	// this causes the app to quit when the last window is closed.
				shouldTick = false;

			auto stopTime = std::chrono::high_resolution_clock::now();
			dt = std::chrono::duration<float, std::chrono::seconds::period>(stopTime - startTime).count();
		}

		OnStop();
		vkDestroyInstance(vulkanInstance, nullptr);
		vulkanInstance = nullptr;
		glfwTerminate();
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
		appInfo.apiVersion = VK_API_VERSION_1_0;

		//CreateInfo
		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;

		createInfo.enabledLayerCount = 0;

		VkResult result = vkCreateInstance(&createInfo, nullptr, &vulkanInstance);
		check_vk_result(result);

		return result == VK_SUCCESS;
	}
}