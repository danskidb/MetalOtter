#include "Otter/Application.hpp"
#include <chrono>

namespace Otter {

	Application::Application()
	{
	}

	Application::~Application()
	{
	}

	void Application::Run(int argc, char* argv[], char* envp[])
	{
		OnStart();

		float dt = 0.0f;
		while (true) {
			auto startTime = std::chrono::high_resolution_clock::now();

			OnTick(dt);

			auto stopTime = std::chrono::high_resolution_clock::now();
			dt = std::chrono::duration<float, std::chrono::seconds::period>(stopTime - startTime).count();
		}

		OnStop();
	}
}