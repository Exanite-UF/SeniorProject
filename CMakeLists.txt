cmake_minimum_required(VERSION 3.28)
project(SeniorProject LANGUAGES C CXX)

# This allows shared libraries to be loaded at different memory addresses
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# This postfixes a "d" to shared library names in DEBUG mode
set(CMAKE_DEBUG_POSTFIX d)

# Check if Interprocedural Optimization (IPO) is supported by the compiler
include(CheckIPOSupported)
check_ipo_supported(RESULT IS_IPO_SUPPORTED)

# Enable IPO if supported
if(IS_IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

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

# IMGUI
add_library(imgui STATIC)

# Sources are in the include directory because imgui uses relative include paths
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ./external/imgui-v1.91.8/include/imgui/*.cpp)
target_sources(imgui PRIVATE ${sources})
target_include_directories(imgui PUBLIC ./external/imgui-v1.91.8/include)
target_link_libraries(imgui PRIVATE glfw)
target_link_libraries(imgui PRIVATE ${OPENGL_LIBRARY})

# Assimp
add_subdirectory(./external/assimp)

# Jolt
set(DOUBLE_PRECISION OFF)
set(GENERATE_DEBUG_SYMBOLS OFF)
set(CROSS_PLATFORM_DETERMINISTIC ON)
set(FLOATING_POINT_EXCEPTIONS_ENABLED OFF)
set(OBJECT_LAYER_BITS 32)
set(USE_SSE4_1 ON)
set(USE_SSE4_2 ON)
set(USE_AVX ON)
set(USE_AVX2 ON)
set(USE_AVX512 OFF)
set(USE_LZCNT ON)
set(USE_TZCNT ON)
set(USE_F16C ON)
set(USE_FMADD ON)

set(PHYSICS_REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/external/jolt")
include(./external/jolt/Jolt/Jolt.cmake)

# Image Magick Core
add_library(magick-core STATIC)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS ./external/image-magick/MagickCore/*.c)
target_sources(magick-core PRIVATE ${sources})
target_include_directories(magick-core PUBLIC ./external/image-magick/MagickCore)

# Image Magick++
add_library(magick++ STATIC)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS ./external/image-magick/Magick++/lib/*.cpp)
target_sources(magick++ PRIVATE ${sources})
target_include_directories(magick++ PUBLIC ./external/image-magick/Magick++/lib)
target_link_libraries(magick++ PRIVATE magick-core)

# --- First party ---

add_subdirectory(./projects/VoxelRenderer)
