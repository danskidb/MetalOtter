#include "Otter/Core/Application.hpp"
#include <chrono>
#include <iostream>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <loguru.hpp>

namespace Otter {

	Application::Application()
	{
	}

	Application::~Application()
	{
		windows.clear();
	}

	void Application::Run(int argc, char* argv[], char* envp[])
	{
		glfwInit();	
		loguru::init(argc, argv);
		loguru::add_file("otter.log", loguru::FileMode::Truncate, loguru::Verbosity_MAX);
		OnStart();

		uint32_t extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
		std::vector<VkExtensionProperties> extensions(extensionCount);
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

		LOG_F(INFO, "%u vulkan extensions supported", extensionCount);
		for (const auto& extension : extensions)
			LOG_F(INFO, "\t %s", extension.extensionName);

		float dt = 0.0f;
		while (shouldTick)
		{
			auto startTime = std::chrono::high_resolution_clock::now();

			OnTick(dt);
			for(auto window : windows)
			{
				window->OnTick();
				if (window->ShouldBeDestroyed())
					DestroyWindow(window);
			}
			
			glfwPollEvents();
			if (windows.empty())	// this causes the app to quit when the last window is closed.
				shouldTick = false;

			auto stopTime = std::chrono::high_resolution_clock::now();
			dt = std::chrono::duration<float, std::chrono::seconds::period>(stopTime - startTime).count();
		}

		OnStop();
		glfwTerminate();
	}

	bool Application::CreateWindow(glm::vec2 size, std::string title)
	{
		std::shared_ptr<Otter::Window> window = std::make_shared<Otter::Window>(size, title);
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
}