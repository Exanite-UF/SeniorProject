cmake_minimum_required(VERSION 3.28)
project(SeniorProject LANGUAGES CXX)

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
add_subdirectory(./external/glew/build/cmake)

# --- First party ---
add_subdirectory(./projects/VoxelRenderer)
