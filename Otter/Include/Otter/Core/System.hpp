#pragma once
#include "Types.hpp"
#include <set>

namespace Otter
{
	class Coordinator;

	class System
	{
	public:
		std::set<Entity> entities;
		Coordinator* coordinator;

		virtual void OnStart() = 0;
		virtual void OnStop() = 0;
		virtual void OnTick(float deltaTime) {};
	};
}