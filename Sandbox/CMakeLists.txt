add_custom_target(EngineAssets
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/Sandbox/Assets
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Otter/Assets ${CMAKE_BINARY_DIR}/Sandbox/Assets
	COMMENT "copying ${CMAKE_SOURCE_DIR}/Otter/Assets to ${CMAKE_BINARY_DIR}/Sandbox/Assets"
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)
add_custom_target(AppAssets
	COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/Sandbox/Assets
	COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/Sandbox/Assets ${CMAKE_BINARY_DIR}/Sandbox/Assets
	COMMENT "copying ${CMAKE_SOURCE_DIR}/Sandbox/Assets to ${CMAKE_BINARY_DIR}/Sandbox/Assets"
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

add_executable(Sandbox 
	Source/SandboxApp.cpp
	Source/MainWindow.cpp
	Source/SecondWindow.cpp
)
target_include_directories(Sandbox PRIVATE Include)
add_dependencies(Sandbox EngineAssets)
add_dependencies(Sandbox AppAssets)

target_include_directories(Sandbox PUBLIC ../Otter/Include)
target_link_libraries(Sandbox PUBLIC Otter)