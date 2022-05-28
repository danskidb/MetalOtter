#pragma once
#include <vector>
#include <string>
#include <filesystem>
#include "glslang/Include/glslang_c_interface.h"
#include "vulkan/vulkan.h"

namespace Otter
{
	class ShaderUtilities
	{
	public:
		static bool LoadShaderCode(std::string path, std::vector<uint32_t>& result);
		static VkShaderModule LoadShaderModule(std::string path, VkDevice logicalDevice); // Loads a new shader as a vulkan module for a logical device. Call vkDestroyShaderModule after use.

	private:
		static std::vector<uint32_t> CompileShaderToSPIRV_Vulkan(const glslang_stage_t stage, const char* shaderSource, const char* fileName);
		static glslang_stage_t FindShaderLanguage(std::filesystem::path &filePath);
	};
}