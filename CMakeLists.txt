cmake_minimum_required(VERSION 3.28)
project(SeniorProject LANGUAGES C CXX)

# From https://github.com/DenizThatMenace/More-Modern-CMake/blob/master/example-3.12/CMakeLists.txt
# Always use '-fPIC'/'-fPIE' option.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Globally disable shared library prefix for Windows
if (WIN32)
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

# --- Third party ---

# OpenGL
find_package(OpenGL REQUIRED COMPONENTS OpenGL)

# GLFW
add_subdirectory(./external/glfw)

# GLM
add_subdirectory(./external/glm)

# GLEW
add_library(glew SHARED)

target_compile_definitions(glew PRIVATE GLEW_BUILD)

target_sources(glew PRIVATE ./external/glew/src/glew.c)
target_include_directories(glew PUBLIC ./external/glew/include)
target_link_libraries(glew PRIVATE ${OPENGL_LIBRARY})

# --- First party ---
add_subdirectory(./projects/VoxelRenderer)
