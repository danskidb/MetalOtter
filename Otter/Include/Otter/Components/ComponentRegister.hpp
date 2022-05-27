#pragma once
#include "Otter/Components/Transform.hpp"

namespace Otter
{
	class Coordinator;
	class ComponentRegister
	{
	public:
		static inline void RegisterComponentsWithCoordinator(Coordinator* coordinator)
		{
			coordinator->RegisterComponent<Components::Transform>();
		}
	};
}