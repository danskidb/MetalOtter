#pragma once
#include <string>

#include <loguru.hpp>
#include <memory>

namespace Otter::Components
{
	class MeshRenderer
	{
	public:
		std::string meshPath;
		std::string texturePath;
		std::string shader = "Simple";

		MeshRenderer() {}

	 	MeshRenderer(std::string meshPath, std::string texturePath)
		{
			this->meshPath = meshPath;
			this->texturePath = texturePath;
		}
	};
}