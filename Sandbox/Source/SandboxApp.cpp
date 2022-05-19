#include "SandboxApp.hpp"
#include "MainWindow.hpp"
#include "SecondWindow.hpp"

namespace Sandbox {

	SandboxApp::SandboxApp() : Application()
	{
		appName = "SandboxApp";
	}

	void SandboxApp::OnStart()
	{
		CreateWindow<Sandbox::MainWindow>({480, 640}, "Main Window");
		CreateWindow<Sandbox::SecondWindow>({640, 480}, "Second Window");
	}

	void SandboxApp::OnTick(float deltaTime)
	{
	}

	void SandboxApp::OnStop()
	{
	}
}