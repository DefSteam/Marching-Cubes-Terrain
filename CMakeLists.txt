cmake_minimum_required(VERSION 3.20)

project(MarchingCubesTerrain VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MCT_BUILD_IMGUI_DEMO "Build ImGui demo window" OFF)

add_subdirectory("Terrain Generator")
