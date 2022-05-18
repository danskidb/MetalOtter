add_compile_definitions(GLFW_INCLUDE_VULKAN)
add_compile_definitions(GLM_FORCE_RADIANS)
add_compile_definitions(GLM_FORCE_DEPTH_ZERO_TO_ONE)

set(IMGUI_DIR ../Libraries/imgui)

add_library(Otter
	../Libraries/loguru/loguru.cpp
	${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
	${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
	${IMGUI_DIR}/imgui.cpp
	${IMGUI_DIR}/imgui_draw.cpp
	${IMGUI_DIR}/imgui_demo.cpp
	${IMGUI_DIR}/imgui_tables.cpp
	${IMGUI_DIR}/imgui_widgets.cpp
	Source/Core/Application.cpp
	Source/Core/Window.cpp
)
target_include_directories(Otter PRIVATE Include)
target_include_directories(Otter PUBLIC ../Libraries/loguru)
target_include_directories(Otter PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends ..)

find_package(Vulkan REQUIRED)
target_link_libraries(Otter PUBLIC Vulkan::Vulkan)
target_link_libraries(Otter PUBLIC glm::glm)
target_link_libraries(Otter PUBLIC glfw)