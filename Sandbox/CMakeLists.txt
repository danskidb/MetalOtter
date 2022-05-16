add_executable(Sandbox Source/SandboxApp.cpp)
target_include_directories(Sandbox PRIVATE Include)

target_include_directories(Sandbox PUBLIC ../Otter/Include)
target_link_libraries(Sandbox PUBLIC Otter)