#include "MainWindow.hpp"

namespace Sandbox
{
	bool show_demo_window = true;
	void MainWindow::DrawImGui()
	{
		ImGui::ShowDemoWindow(&show_demo_window);
	}
}