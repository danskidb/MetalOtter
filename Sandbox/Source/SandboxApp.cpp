#include "SandboxApp.hpp"

namespace Sandbox {

	void SandboxApp::OnStart()
	{
		CreateWindow({800, 600}, "SandboxApp");
	}

	void SandboxApp::OnTick(float deltaTime)
	{
	}

	void SandboxApp::OnStop()
	{
	}
}