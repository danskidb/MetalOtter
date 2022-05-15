#pragma once

namespace Otter {

	class Application
	{
	public:
		Application();
		virtual ~Application();

		void Run(int argc, char* argv[], char* envp[]);

		virtual void OnStart() = 0;
		virtual void OnTick(float deltaTime) = 0;
		virtual void OnStop() = 0;
	};

	// To be defined in CLIENT
	Application* CreateApplication();
}