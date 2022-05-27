#pragma once
#include "Otter/Core/System.hpp"

namespace Otter::Systems
{
	class Event;
	class TemplateSystem : public System
	{
	public:
		virtual void OnStart();
		virtual void OnStop();
	};
}
