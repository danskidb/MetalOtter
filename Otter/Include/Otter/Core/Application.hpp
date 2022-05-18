#pragma once

#include <vector>
#include <string>
#include "Otter/Core/Window.hpp"
#include "glm/vec2.hpp"

namespace Otter 
{
	class Application
	{
	public:
		Application();
		virtual ~Application();

		void Run(int argc, char* argv[], char* envp[]);

		virtual void OnStart() = 0;
		virtual void OnTick(float deltaTime) = 0;
		virtual void OnStop() = 0;

		// Creates a new window, returns success.
		bool CreateWindow(glm::vec2 size, std::string title);
		bool DestroyWindow(std::shared_ptr<Otter::Window> window);

	private:
		std::vector<std::shared_ptr<Otter::Window>> windows;
		bool windowWasDestroyed = true;
		bool shouldTick = true;
	};

	// To be defined in CLIENT
	Application* CreateApplication();
}