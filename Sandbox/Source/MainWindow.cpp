#include "MainWindow.hpp"
#include "imgui.h"

namespace Sandbox
{
	void MainWindow::OnDrawImGui()
	{
		ImGui::Begin("Hello, world!");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();	
	}
}