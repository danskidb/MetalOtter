#include "Otter/Core/Application.hpp"
#include <chrono>
#include <iostream>
#include <GLFW/glfw3.h>
#include <loguru.hpp>

namespace Otter {

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

		if(glfwInit() != GLFW_TRUE)
		{
			LOG_F(ERROR, "Failed to initialize GLFW");
			return;
		}

		if(!CreateVulkanInstance())
		{
			LOG_F(ERROR, "Failed to create vulkan instance");
			return;
		}

		OnStart();

		// uint32_t extensionCount = 0;
		// vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		// std::vector<VkExtensionProperties> extensions(extensionCount);
		// vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		// LOG_F(INFO, "%u vulkan extensions supported", extensionCount);
		// for (const auto& extension : extensions)
		// 	LOG_F(INFO, "\t %s", extension.extensionName);

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

	bool Application::CreateWindow(glm::vec2 size, std::string title)
	{
		std::shared_ptr<Otter::Window> window = std::make_shared<Otter::Window>(size, title, vulkanInstance);
		if (!window->IsValid())
			return false;

		windows.push_back(window);
		return true;
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

		return result == VK_SUCCESS;
	}
}