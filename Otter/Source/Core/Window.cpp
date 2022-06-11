#include "Otter/Core/Window.hpp"
#include "Otter/Components/ComponentRegister.hpp"
#include "loguru.hpp"

namespace Otter 
{
	// static void framebufferResizeCallback(GLFWwindow* handle, int width, int height)
	// {
	// 	auto window = reinterpret_cast<Otter::Window*>(glfwGetWindowUserPointer(handle));
	// 	window->GetRenderer()->InvalidateFramebuffer();
	// }

	Window::Window(glm::vec2 size, std::string title, VkInstance vulkanInstance, bool imGuiAllowed)
	{
		this->vulkanInstance = vulkanInstance;
		this->title = title;
		
		handle = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, size.x, size.y, SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);
		// Setup Window and its callbacks.
		// glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		// handle = glfwCreateWindow((int)size.x, (int)size.y, title.c_str(), nullptr, nullptr);
		LOG_F(INFO, "Initializing new window \'%s\' with size %gx%g", title.c_str(), size.x, size.y);
		
		// glfwSetWindowUserPointer(handle, this);
		// glfwSetFramebufferSizeCallback(handle, framebufferResizeCallback);

		// Set up Components
		ComponentRegister::RegisterComponentsWithCoordinator(&coordinator);

		// Set up Systems
		// renderer = coordinator.RegisterSystem<Systems::Renderer>();
		// {
		// 	Signature signature;
		// 	signature.set(coordinator.GetComponentType<Components::Transform>());
		// 	coordinator.SetSystemSignature<Systems::Renderer>(signature);
		// }
		// renderer->SetWindowHandle(handle);
		// renderer->SetVulkanInstance(vulkanInstance);
		// renderer->SetFrameBufferResizedCallback([this](glm::vec2 newSize) {
		// 	OnWindowResized(newSize);
		// });
		// if(imGuiAllowed)
		// {
		// 	renderer->SetImGuiAllowed();
		// 	renderer->SetDrawImGuiCallback([this]() {
		// 		OnDrawImGui();
		// 	});
		// }
		// renderer->OnStart();
		// systems.push_back(renderer);

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
			return false;//glfwWindowShouldClose(handle) != 0;
	}

	void Window::OnTick(float deltaTime)
	{
		if (!IsValid())
			return;

		for(const auto system : systems)
			system->OnTick(deltaTime);
	}

	void Window::OnWindowResized(glm::vec2 size)
	{
		LOG_F(INFO, "Window %s was resized to %gx%g", title.c_str(), size.x, size.y);
	}
}