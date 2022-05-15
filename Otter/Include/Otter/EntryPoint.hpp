#pragma once

extern Otter::Application* Otter::CreateApplication();

int main(int argc, char* argv[], char* envp[])
{
	auto app = Otter::CreateApplication();
	app->Run(argc, argv, envp);
	delete app;
}