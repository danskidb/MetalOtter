#pragma once
#include "Otter/Systems/TemplateSystem.hpp"
#include "Otter/Components/Transform.hpp"

namespace Otter
{
	class Coordinator;
	class SystemRegister
	{
	public:
		static inline void RegisterSystemsWithCoordinator(Coordinator* coordinator)
		{
			auto templateSystem = coordinator->RegisterSystem<Systems::TemplateSystem>();
			{
				Signature signature;
				signature.set(coordinator->GetComponentType<Components::Transform>());
				coordinator->SetSystemSignature<Systems::TemplateSystem>(signature);
			}
			templateSystem->OnStart();
		}
	};
}