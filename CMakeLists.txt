cmake_minimum_required(VERSION 3.28)
project(SeniorProject LANGUAGES C CXX)

# This allows shared libraries to be loaded at different memory addresses
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# This postfixes a "d" to shared library names in DEBUG mode
set(CMAKE_DEBUG_POSTFIX d)

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
add_library(glew STATIC)

target_compile_definitions(glew PUBLIC GLEW_STATIC)

target_sources(glew PUBLIC ./external/glew-v2.2.0/src/glew.c)
target_include_directories(glew PUBLIC ./external/glew-v2.2.0/include)
target_link_libraries(glew PRIVATE ${OPENGL_LIBRARY})

# --- First party ---
add_subdirectory(./projects/VoxelRenderer)
