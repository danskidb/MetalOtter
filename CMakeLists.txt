cmake_minimum_required(VERSION 3.5)
project(OtterApp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_subdirectory(Libraries)
add_subdirectory(Otter)
add_subdirectory(Sandbox)

