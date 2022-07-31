#pragma once
#include "Otter/Components/Transform.hpp"
#include "Otter/Components/MeshRenderer.hpp"

namespace Otter
{
	class Coordinator;
	class ComponentRegister
	{
	public:
		static inline void RegisterComponentsWithCoordinator(Coordinator* coordinator)
		{
			coordinator->RegisterComponent<Components::Transform>();
			coordinator->RegisterComponent<Components::MeshRenderer>();
		}
	};
}