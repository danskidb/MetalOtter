#pragma once

#include "glm/vec2.hpp"
#include "GLFW/glfw3.h"
#include <string>

namespace Otter
{
	class Window 
	{
	public:
		Window(glm::vec2 size, std::string title);
		~Window();

		inline bool IsValid() { return handle != nullptr; }

		void OnTick() {}
		bool ShouldBeDestroyed();

	private:
		GLFWwindow* handle;
	};
}