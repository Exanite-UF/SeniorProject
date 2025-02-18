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

target_link_libraries(VoxelRenderer PRIVATE ${OPENGL_LIBRARY})
target_link_libraries(VoxelRenderer PRIVATE glfw)
target_link_libraries(VoxelRenderer PRIVATE glm::glm)
target_link_libraries(VoxelRenderer PRIVATE glew)
target_link_libraries(VoxelRenderer PRIVATE imgui)
#target_link_libraries(VoxelRenderer PRIVATE assimp)
#target_link_libraries(VoxelRenderer PRIVATE Jolt)
target_link_libraries(VoxelRenderer PRIVATE stb)

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
