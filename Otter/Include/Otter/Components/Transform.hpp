#pragma once
#include "glm/mat4x4.hpp"

namespace Otter::Components
{
	struct Transform
	{
		glm::mat4x4 data;

		glm::vec3 GetPosition() { return glm::vec3(); }
	};
}