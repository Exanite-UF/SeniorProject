cmake_minimum_required(VERSION 3.28)
project(VoxelRenderer LANGUAGES CXX)

# --- Build ---

add_executable(VoxelRenderer)
set_target_properties(VoxelRenderer
        PROPERTIES
        CXX_STANDARD 23
        OUTPUT_NAME "VoxelRenderer"
)

file(GLOB sources CONFIGURE_DEPENDS ./src/*.cpp)
target_sources(VoxelRenderer PRIVATE ${sources})

target_link_libraries(VoxelRenderer PRIVATE ${OPENGL_LIBRARY})
target_link_libraries(VoxelRenderer PRIVATE glfw)
target_link_libraries(VoxelRenderer PRIVATE glm::glm)
target_link_libraries(VoxelRenderer PRIVATE glew)

# --- Package ---

set(CPACK_PACKAGE_NAME "VoxelRenderer")
set(CPACK_OUTPUT_CONFIG_FILE "${CMAKE_BINARY_DIR}/CPackConfig-VoxelRenderer.cmake")
install(TARGETS VoxelRenderer)

include(CPack)
