cmake_minimum_required(VERSION 3.28)
project(VoxelRenderer LANGUAGES CXX)

# --- Build ---

add_executable(VoxelRenderer)
set_target_properties(VoxelRenderer
    PROPERTIES
    CXX_STANDARD 23
    OUTPUT_NAME "VoxelRenderer"
)

file(GLOB_RECURSE sources CONFIGURE_DEPENDS ./src/*.cpp)
target_sources(VoxelRenderer PRIVATE ${sources})
target_include_directories(VoxelRenderer PRIVATE .)

target_link_libraries(VoxelRenderer PRIVATE glm::glm)
target_compile_definitions(VoxelRenderer PRIVATE GLM_ENABLE_EXPERIMENTAL)

target_link_libraries(VoxelRenderer PRIVATE ${OPENGL_LIBRARY})
target_link_libraries(VoxelRenderer PRIVATE glfw)
target_link_libraries(VoxelRenderer PRIVATE glew)
target_link_libraries(VoxelRenderer PRIVATE imgui)
target_link_libraries(VoxelRenderer PRIVATE assimp)
target_link_libraries(VoxelRenderer PRIVATE Jolt)
target_link_libraries(VoxelRenderer PRIVATE stb)
target_link_libraries(VoxelRenderer PRIVATE Tracy::TracyClient)

# Boost is a CGAL dependency
target_link_libraries(VoxelRenderer PRIVATE boost_config)
target_link_libraries(VoxelRenderer PRIVATE boost_predef)
target_link_libraries(VoxelRenderer PRIVATE boost_type_traits)
target_link_libraries(VoxelRenderer PRIVATE boost_mpl)
target_link_libraries(VoxelRenderer PRIVATE boost_functional)
target_link_libraries(VoxelRenderer PRIVATE boost_compute)
target_link_libraries(VoxelRenderer PRIVATE boost_math)
target_link_libraries(VoxelRenderer PRIVATE cgal)

target_link_libraries(VoxelRenderer PRIVATE extra)

# --- Copy content ---

add_custom_target(VoxelRenderer_CopyContent)
add_custom_command(
    TARGET VoxelRenderer_CopyContent POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
    ${CMAKE_CURRENT_SOURCE_DIR}/content
    $<TARGET_FILE_DIR:VoxelRenderer>/content
)
add_dependencies(VoxelRenderer VoxelRenderer_CopyContent)

# --- Package ---

set(CPACK_PACKAGE_NAME "VoxelRenderer")
set(CPACK_OUTPUT_CONFIG_FILE "${CMAKE_BINARY_DIR}/CPackConfig-VoxelRenderer.cmake")
install(TARGETS VoxelRenderer)

include(CPack)
