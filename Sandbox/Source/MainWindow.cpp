#include "MainWindow.hpp"
#include "imgui.h"
#include "Otter/Components/MeshRenderer.hpp"

namespace Sandbox
{
	MainWindow::MainWindow(glm::vec2 size, std::string title, bool imGuiAllowed) : Otter::Window(size, title, imGuiAllowed)
	{
		Otter::Components::MeshRenderer mr = { "Assets/Meshes/viking_room.obj", "Assets/Textures/viking_room.png" };
		Otter::Entity entity = coordinator.CreateEntity();
		coordinator.AddComponent(entity, mr);
	}

	void MainWindow::OnDrawImGui()
	{
		ImGui::Begin("Hello, world!");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();	
	}
}