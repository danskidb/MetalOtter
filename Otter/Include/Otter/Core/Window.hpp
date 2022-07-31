#pragma once
#include <string>
#include <vector>
#include "SDL.h"
#include "Otter/Core/Coordinator.hpp"
#include "Otter/Systems/Renderer.hpp"

namespace Otter
{
	class Window 
	{
	public:
		Window(glm::vec2 size, std::string title, bool imGuiAllowed);
		virtual ~Window();

		virtual void OnStart();

		inline bool IsValid() { return handle != nullptr && initialized; }
		inline uint32_t GetWindowId() { return windowId; }
		bool ShouldBeDestroyed();

		virtual void OnTick(float deltaTime);
		virtual void OnSDLEvent(SDL_Event* event);
		virtual void OnWindowEvent(SDL_WindowEvent* event);
		virtual void OnDrawImGui() {}
		virtual void OnWindowResized(glm::vec2 size);

	protected:
		Coordinator coordinator;

	private:
		bool initialized = false;
		bool shouldBeDestroyed = false;
		std::string title;

		std::vector<std::shared_ptr<Otter::System>> systems;
		std::shared_ptr<Systems::Renderer> renderer;

		SDL_Window* handle;
		uint32_t windowId;
		Vec2D size = {0, 0};

		//Window focus
        bool mouseFocus = false;
        bool keyboardFocus = false;
        bool fullScreen = false;
        bool minimized = false;
        bool shown = false;
	};
}