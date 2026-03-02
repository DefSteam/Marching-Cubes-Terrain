include(FetchContent)

set(FETCHCONTENT_UPDATES_DISCONNECTED ON)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG 3.4
)

FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad.git
  GIT_TAG v2.0.8
)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG v1.91.9b
)

FetchContent_MakeAvailable(glfw glad imgui)

glad_add_library(glad REPRODUCIBLE API gl:core=4.5)

set(IMGUI_SOURCES
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

if(MCT_BUILD_IMGUI_DEMO)
  list(APPEND IMGUI_SOURCES ${imgui_SOURCE_DIR}/imgui_demo.cpp)
endif()

add_executable(marching_cubes_terrain
  main.cpp
  ${IMGUI_SOURCES}
)

target_include_directories(marching_cubes_terrain PRIVATE
  .
  ../libs/FastNoiseLite
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(marching_cubes_terrain PRIVATE glfw glad)

target_compile_definitions(marching_cubes_terrain PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)

if(UNIX AND NOT APPLE)
  find_package(OpenGL REQUIRED)
  target_link_libraries(marching_cubes_terrain PRIVATE OpenGL::GL)
endif()
