add_library(Otter
	../Libraries/loguru/loguru.cpp
	Source/Core/Application.cpp
	Source/Core/Window.cpp
)
target_include_directories(Otter PRIVATE Include)
target_include_directories(Otter PUBLIC ../Libraries/loguru)

find_package(Vulkan REQUIRED)
target_link_libraries(Otter PUBLIC Vulkan::Vulkan)
target_link_libraries(Otter PUBLIC glm::glm)
target_link_libraries(Otter PUBLIC glfw)