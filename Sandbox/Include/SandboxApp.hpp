#pragma once

#include "Otter.hpp"

namespace Sandbox {
	class SandboxApp : public Otter::Application
	{
	public:
		void OnStart() override;
		void OnTick(float deltaTime) override;
		void OnStop() override;
	};
}

Otter::Application* Otter::CreateApplication()
{
	return new Sandbox::SandboxApp();
}