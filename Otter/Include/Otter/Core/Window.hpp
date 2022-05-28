#pragma once
#include <string>
#include <vector>
#include "Otter/Core/Coordinator.hpp"
#include "Otter/Systems/Renderer.hpp"

namespace Otter
{
	class Window 
	{
	public:
		Window(glm::vec2 size, std::string title, VkInstance vulkanInstance);
		virtual ~Window();

		inline bool IsValid() { return handle != nullptr && initialized; }
		inline std::shared_ptr<Systems::Renderer> GetRenderer() { return renderer; }
		bool ShouldBeDestroyed();

		virtual void OnTick(float deltaTime);
		virtual void OnWindowResized(glm::vec2 size);

	private:
		Coordinator coordinator;
		std::vector<std::shared_ptr<Otter::System>> systems;

		bool initialized = false;
		std::string title;

		GLFWwindow* handle;
		VkInstance vulkanInstance;

		std::shared_ptr<Systems::Renderer> renderer;
	};
}