#include "Otter/Core/Window.hpp"

namespace Otter 
{
	Window::Window(glm::vec2 size, std::string title)
	{
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		handle = glfwCreateWindow((int)size.x, (int)size.y, title.c_str(), nullptr, nullptr);
	}

	Window::~Window()
	{
		if (!IsValid())
			return;
		
		glfwDestroyWindow(handle);
	}

	bool Window::ShouldBeDestroyed()
	{
		if (!IsValid())
			return true;
		else
			return glfwWindowShouldClose(handle) != 0;
	}
}