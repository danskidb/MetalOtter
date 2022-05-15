add_executable(Sandbox Source/SandboxApp.cpp)
target_include_directories(Sandbox PRIVATE Include)

target_include_directories(Sandbox PUBLIC ../Otter/Include)
target_link_libraries(Sandbox PUBLIC Otter)

target_include_directories(Sandbox PUBLIC ../Libraries/metal-cpp/include)
target_link_libraries(Sandbox INTERFACE metal-cpp)