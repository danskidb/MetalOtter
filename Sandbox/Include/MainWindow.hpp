#pragma once
#include "Otter/Core/Window.hpp"

namespace Sandbox {
	class MainWindow : public Otter::Window
	{
	public:
		MainWindow(glm::vec2 size, std::string title, bool imGuiAllowed) : Otter::Window(size, title, imGuiAllowed) {}

		virtual void OnDrawImGui();
	};

}