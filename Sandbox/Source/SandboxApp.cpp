#include "SandboxApp.hpp"

namespace Sandbox {

	SandboxApp::SandboxApp() : Application()
	{
		appName = "SandboxApp";
	}

	void SandboxApp::OnStart()
	{
		CreateWindow({480, 640}, "Main Window");
		//CreateWindow({640, 480}, "Second Window");
	}

	void SandboxApp::OnTick(float deltaTime)
	{
	}

	void SandboxApp::OnStop()
	{
	}
}