add_library(Otter
	Source/Application.cpp
	Source/OS/Darwin/Window.cpp
)
target_include_directories(Otter PRIVATE Include)

target_link_libraries(Otter PUBLIC METAL_CPP)