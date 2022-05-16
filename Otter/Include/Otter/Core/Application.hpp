#pragma once

#ifdef __APPLE__
#include "AppKit/AppKit.hpp"
#endif

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

#ifdef __APPLE__
	class MyAppDelegate : public NS::ApplicationDelegate
	{
		public:
			~MyAppDelegate();

			NS::Menu* createMenuBar();

			virtual void applicationWillFinishLaunching( NS::Notification* pNotification ) override;
			virtual void applicationDidFinishLaunching( NS::Notification* pNotification ) override;
			virtual bool applicationShouldTerminateAfterLastWindowClosed( NS::Application* pSender ) override;
	};
#endif
}