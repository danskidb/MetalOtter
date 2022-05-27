#include <vector>
#include <string>
#include <filesystem>
#include "glslang/Include/glslang_c_interface.h"

namespace Otter
{
	class ShaderUtilities
	{
	public:
		static bool LoadShader(std::string path, std::vector<uint32_t>& result);

	private:
		static std::vector<uint32_t> CompileShaderToSPIRV_Vulkan(const glslang_stage_t stage, const char* shaderSource, const char* fileName);
		static glslang_stage_t FindShaderLanguage(std::filesystem::path &filePath);
	};
}