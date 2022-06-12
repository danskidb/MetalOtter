cmake_minimum_required(VERSION 3.9)
project(OtterApp)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
enable_language(CXX)

find_package(Git REQUIRED QUIET)
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
	option(GIT_SUBMODULE "Check submodules during build" ON)
	if(GIT_SUBMODULE)
		message(STATUS "Submodule update")
		execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
			WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
			RESULT_VARIABLE GIT_SUBMOD_RESULT)
		if(NOT GIT_SUBMOD_RESULT EQUAL "0")
			message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
		endif()
	endif()
else()
	message(FATAL_ERROR "You must download this repository via Git")
endif()

add_subdirectory(Libraries)
add_subdirectory(Otter)
add_subdirectory(Sandbox)

if(PLATFORM MATCHES "OS64")
	add_subdirectory(Sandbox_iOS)
endif()