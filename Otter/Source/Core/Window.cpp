#include "Otter/Core/Window.hpp"
#include "Otter/Components/ComponentRegister.hpp"
#include "loguru.hpp"

namespace Otter 
{
	static void AssertSDLError(bool condition)
	{
		if (condition)
			return;

		LOG_F(ERROR, "[SDL] %s", SDL_GetError());
		abort();
	}

	Window::Window(glm::vec2 size, std::string title, bool imGuiAllowed)
	{
		this->handle = NULL;
		this->windowId = 0;

		//TODO high DPI support (SDL_WINDOW_ALLOW_HIGHDPI)
		handle = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, size.x, size.y, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
		AssertSDLError(handle != NULL);

		windowId = SDL_GetWindowID(handle);
		AssertSDLError(windowId != 0);

		this->size = {size.x, size.y};
		this->title = title;
		this->mouseFocus = true;
        this->keyboardFocus = true;

		LOG_F(INFO, "Initialized new window \'%s\' with size %gx%g and id %u", title.c_str(), size.x, size.y, windowId);

		// Set up Components
		ComponentRegister::RegisterComponentsWithCoordinator(&coordinator);

		// Set up Systems
		renderer = coordinator.RegisterSystem<Systems::Renderer>();
		{
			Signature signature;
			signature.set(coordinator.GetComponentType<Components::Transform>());
			coordinator.SetSystemSignature<Systems::Renderer>(signature);
		}
		renderer->SetWindowHandle(handle);
		renderer->SetFrameBufferResizedCallback([this](glm::vec2 newSize) {
			OnWindowResized(newSize);
		});
		if(imGuiAllowed)
		{
			renderer->SetImGuiAllowed();
			renderer->SetDrawImGuiCallback([this]() {
				OnDrawImGui();
			});
		}
		renderer->OnStart();
		systems.push_back(renderer);

		// Done. Let's go!
		initialized = true;
	}

	Window::~Window()
	{
		if (!IsValid())
		{
			LOG_F(WARNING, "Window \'%s\' was not valid at destruction time.", title.c_str());
			return;
		}

		for(auto system : systems)
			system->OnStop();

		SDL_DestroyWindow(handle);

		LOG_F(INFO, "Closed window %s", title.c_str());
	}

	bool Window::ShouldBeDestroyed()
	{
		if (!IsValid())
			return true;
		else
			return shouldBeDestroyed;
	}

	void Window::OnTick(float deltaTime)
	{
		if (!IsValid())
			return;

		for(const auto system : systems)
			system->OnTick(deltaTime);
	}

	void Window::OnSDLEvent(SDL_Event* event)
	{
		renderer->OnSDLEvent(event);
	}

	void Window::OnWindowEvent(SDL_WindowEvent* windowEvent)
	{
		switch (windowEvent->event)
        {
        case SDL_WINDOWEVENT_SHOWN:
            shown = true;
            break;
        case SDL_WINDOWEVENT_HIDDEN:
            shown = false;
            break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:	
			size = {windowEvent->data1, windowEvent->data2};
			renderer->InvalidateFramebuffer();
            break;
        case SDL_WINDOWEVENT_EXPOSED:
			renderer->InvalidateFramebuffer();
            break;
        case SDL_WINDOWEVENT_ENTER:
            mouseFocus = true;
            break;
        case SDL_WINDOWEVENT_LEAVE:
            mouseFocus = false;
            break;
        case SDL_WINDOWEVENT_FOCUS_GAINED:
            keyboardFocus = true;
            break;
        case SDL_WINDOWEVENT_FOCUS_LOST:
            keyboardFocus = false;
            break;
        case SDL_WINDOWEVENT_MINIMIZED:
            minimized = true;				//TODO this doesnt go back false after restore?
            break;
        case SDL_WINDOWEVENT_MAXIMIZED:
            minimized = false;
            break;
        case SDL_WINDOWEVENT_RESTORED:
            minimized = false;
            break;
		case SDL_WINDOWEVENT_CLOSE:
            shouldBeDestroyed = true;
            break;
		}

		// LOG_F(INFO, "[%s] Window State: shown=%s mouseFocus=%s keyboardFocus=%s, minimized=%s, shouldBeDestroyed=%s",
		// 	title.c_str(), 
		// 	shown?"true":"false", 
		// 	mouseFocus?"true":"false",
		// 	keyboardFocus?"true":"false",
		// 	minimized?"true":"false",
		// 	shouldBeDestroyed?"true":"false"
		// );
	}

	void Window::OnWindowResized(glm::vec2 size)
	{
		LOG_F(INFO, "Window %s was resized to %gx%g", title.c_str(), size.x, size.y);
	}
}