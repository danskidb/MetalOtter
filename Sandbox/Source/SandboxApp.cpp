#include "SandboxApp.hpp"

namespace Sandbox {

	SandboxApp::SandboxApp() : Application()
	{
		appName = "SandboxApp";
	}

	void SandboxApp::OnStart()
	{
		CreateWindow({720, 1280}, "Main Window");
		CreateWindow({1280, 720}, "Second Window");
	}

	void SandboxApp::OnTick(float deltaTime)
	{
	}

	void SandboxApp::OnStop()
	{
	}
}