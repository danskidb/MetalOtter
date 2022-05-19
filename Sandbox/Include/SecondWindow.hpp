#pragma once
#include "Otter/Core/Window.hpp"

namespace Sandbox {
	class SecondWindow : public Otter::Window
	{
	public:
		SecondWindow(glm::vec2 size, std::string title, VkInstance vulkanInstance) : Otter::Window(size, title, vulkanInstance) {}
		virtual void DrawImGui();
	};

}