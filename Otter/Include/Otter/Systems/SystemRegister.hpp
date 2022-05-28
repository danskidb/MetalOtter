#pragma once
#include "GLFW/glfw3.h"
#include "Otter/Systems/TemplateSystem.hpp"
#include "Otter/Components/Transform.hpp"
#include <vector>
#include <memory>

namespace Otter
{
	class Coordinator;
	class SystemRegister
	{
	public:
		static inline std::vector<std::shared_ptr<Otter::System>> RegisterSystemsWithCoordinator(Coordinator* coordinator, GLFWwindow* window)
		{
			std::vector<std::shared_ptr<Otter::System>> result;

			auto templateSystem = coordinator->RegisterSystem<Systems::TemplateSystem>();
			{
				Signature signature;
				signature.set(coordinator->GetComponentType<Components::Transform>());
				coordinator->SetSystemSignature<Systems::TemplateSystem>(signature);
			}
			templateSystem->SetWindowHandle(window);
			templateSystem->OnStart();
			result.push_back(templateSystem);

			return result;
		}
	};
}