#pragma once

#include <vector>
#include <string>
#include "Otter/Core/Window.hpp"
#include "glm/vec2.hpp"
#include "vulkan/vulkan.h"

namespace Otter 
{
	class Application
	{
	public:
		std::string appName;

		Application();
		virtual ~Application();

		void Run(int argc, char* argv[], char* envp[]);

		virtual void OnStart() = 0;
		virtual void OnTick(float deltaTime) = 0;
		virtual void OnStop() = 0;

		template<typename T>
		bool CreateWindow(glm::vec2 size, std::string title);
		bool DestroyWindow(std::shared_ptr<Otter::Window> window);
		bool CreateVulkanInstance();

	private:
		VkInstance vulkanInstance;
		std::vector<std::shared_ptr<Otter::Window>> windows;
		bool windowWasDestroyed = true;
		bool shouldTick = true;

	};

	// To be defined in CLIENT
	Application* CreateApplication();

	template<typename T>
	bool Application::CreateWindow(glm::vec2 size, std::string title)
	{
		std::shared_ptr<T> window = std::make_shared<T>(size, title, vulkanInstance);
		if (!window->IsValid())
			return false;

		windows.push_back(window);
		return true;
	}
}