#include "Otter/Core/Application.hpp"
#include <chrono>
#include <iostream>
#include <loguru.hpp>
#include "SDL.h"
#include "glslang/Include/glslang_c_interface.h"

namespace Otter {

	Application::Application()
	{
		appName = "OtterApplication";
	}

	Application::~Application()
	{
	}

	void Application::Run(int argc, char* argv[], char* envp[])
	{
		loguru::init(argc, argv);
		loguru::add_file("otter.log", loguru::FileMode::Truncate, loguru::Verbosity_MAX);

		SDL_Init(SDL_INIT_EVERYTHING);
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
				switch(event.type)
				{
					case SDL_QUIT:
					{
						for(auto window : windows)
							windowsToBeDestroyed.push_back(window);
					}
					case SDL_WINDOWEVENT:
					{
						for(auto window : windows)
							if(window->GetWindowId() == event.window.windowID)
								window->OnWindowEvent(&event.window);
					}
					default:
					{
						for(auto window : windows)
							window->OnSDLEvent(&event);
					}
				}
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
}