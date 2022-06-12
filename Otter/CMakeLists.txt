add_compile_definitions(GLM_FORCE_RADIANS)

set(IMGUI_DIR ../Libraries/imgui)

add_library(Otter
	../Libraries/loguru/loguru.cpp
	${IMGUI_DIR}/backends/imgui_impl_sdl.cpp
	${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_demo.cpp
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
	Source/Core/Application.cpp
	Source/Core/Window.cpp
	Source/Systems/Renderer.cpp
	Source/Systems/TemplateSystem.cpp
	Source/Utilities/MD5.cpp
	Source/Utilities/ShaderUtilities.cpp
)
target_include_directories(Otter PRIVATE Include)
target_include_directories(Otter PUBLIC ../Libraries/stb)
target_include_directories(Otter PUBLIC ../Libraries/loguru)
target_include_directories(Otter PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends ..)

find_package(Vulkan REQUIRED)
target_link_libraries(Otter 
	PUBLIC Vulkan::Vulkan
	PUBLIC SDL2::SDL2-static
	PUBLIC glm::glm
	PUBLIC glslang
	PUBLIC SPIRV
	PUBLIC VulkanMemoryAllocator
)