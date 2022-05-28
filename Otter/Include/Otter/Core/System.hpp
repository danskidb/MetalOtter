#pragma once
#include "GLFW/glfw3.h"
#include "Types.hpp"
#include <set>

namespace Otter
{
	class System
	{
	public:
		std::set<Entity> mEntities;

		virtual void OnStart() = 0;
		virtual void OnStop() = 0;
		virtual void OnTick(float deltaTime) {};

		inline void SetWindowHandle(GLFWwindow* windowHandle) { this->windowHandle = windowHandle; }

	protected:
		GLFWwindow* windowHandle;
	};
}