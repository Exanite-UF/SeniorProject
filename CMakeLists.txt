cmake_minimum_required(VERSION 3.28)
project(VoxelRendering LANGUAGES CXX)

# From https://github.com/DenizThatMenace/More-Modern-CMake/blob/master/example-3.12/CMakeLists.txt
# Always use '-fPIC'/'-fPIE' option.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Globally disable shared library prefix for Windows
if (WIN32)
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

# --- Third party ---

# OpenGL

# GLFW

# GLM

# GLEW

# --- First party ---
