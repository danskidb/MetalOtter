#pragma once
#include "Otter/Core/Window.hpp"

namespace Sandbox {
	class SecondWindow : public Otter::Window
	{
	public:
		SecondWindow(glm::vec2 size, std::string title, VkInstance vulkanInstance, bool enableImGui) : Otter::Window(size, title, vulkanInstance, enableImGui) {}
		virtual void DrawImGui();
	};

}