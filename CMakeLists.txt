cmake_minimum_required(VERSION 3.28)
project(SeniorProject LANGUAGES C CXX)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_definitions(DEBUG)
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_definitions(RELEASE)
else()
    # Default to release
    add_compile_definitions(RELEASE)
endif()

# This allows shared libraries to be loaded at different memory addresses
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# This postfixes a "d" to shared library names in DEBUG mode
set(CMAKE_DEBUG_POSTFIX d)

# Globally disable shared library prefix for Windows
if (WIN32)
    set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

# Set IPO policy
# The NEW policy enables IPO flags for all compilers if possible and to error if not
# We check for compatibility below so using the new policy is safe
cmake_policy(SET CMP0069 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0069 NEW)

# Enable Inter-Procedural Optimization (IPO) if supported by the compiler
include(CheckIPOSupported)
check_ipo_supported(RESULT IS_IPO_SUPPORTED)
if((IS_IPO_SUPPORTED) AND NOT (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC"))
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
endif()

# Enable architecture specific optimizations
if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-march=native)
endif()

# Support Windows 10 or later
# See: https://learn.microsoft.com/en-us/cpp/porting/modifying-winver-and-win32-winnt
if (WIN32)
    add_definitions(-D_WIN32_WINNT=0x0A00)
endif()

# Set option variable policy
# The NEW policy enables the use of the option() function
cmake_policy(SET CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)

# Hardcoded AddressSanitizer toggle
if (OFF)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
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

## Image Magick Core
#add_library(magick-core STATIC)
#
#file(GLOB_RECURSE sources CONFIGURE_DEPENDS ./external/image-magick/MagickCore/*.c)
#target_sources(magick-core PRIVATE ${sources})
#target_include_directories(magick-core PUBLIC ./external/image-magick/MagickCore)
#
## Image Magick++
#add_library(magick++ STATIC)
#
#file(GLOB_RECURSE sources CONFIGURE_DEPENDS ./external/image-magick/Magick++/lib/*.cpp)
#target_sources(magick++ PRIVATE ${sources})
#target_include_directories(magick++ PUBLIC ./external/image-magick/Magick++/lib)
#target_link_libraries(magick++ PRIVATE magick-core)

# Stb
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE ./external/stb)

# Boost
add_subdirectory(./external/boost-v1.87.0)

# CGAL
add_library(cgal INTERFACE)
target_include_directories(cgal INTERFACE ./external/cgal-v6.0.1/include)

# Tracy
# Enable Tracy profiling for a program by linking it with Tracy::TracyClient

# Only enable Tracy profiling if SP_ENABLE_PROFILING=ON
if(SP_ENABLE_PROFILING)
    set(TRACY_ENABLE ON)
else ()
    set(TRACY_ENABLE OFF)
endif()

set(TRACY_ON_DEMAND ON)
set(TRACY_CALLSTACK ON)

if (DEBUGINFOD_LIBRARY)
    message(STATUS "Found libdebuginfod. Setting TRACY_DEBUGINFOD to ON")
    set(TRACY_DEBUGINFOD ON)
endif()

add_subdirectory(./external/tracy)

# Tracy UI
# Run by running the tracy-profiler target
if(NOT ((DEFINED ENV{GITHUB_ACTIONS}) OR (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")))
    add_subdirectory(./external/tracy/profiler)
endif()

# debuginfod
find_library(DEBUGINFOD_LIBRARY NAMES debuginfod)

# Extra libraries
add_library(extra STATIC)
target_include_directories(extra PUBLIC ./external/extra/include)
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ./external/extra/src/*.cpp)
target_sources(extra PRIVATE ${sources})
file(GLOB_RECURSE sources CONFIGURE_DEPENDS ./external/extra/src/*.c)
target_sources(extra PRIVATE ${sources})

# --- First party ---

add_subdirectory(./projects/VoxelRenderer)
