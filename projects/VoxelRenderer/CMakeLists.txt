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


if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/__header.h)
    message(FATAL_ERROR "File \"${CMAKE_CURRENT_BINARY_DIR}/__header.h\" found, \
    this should never be created, remove!")
endif()

add_custom_target(
  my_custom_target_that_always_runs ALL
  DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/__header.h
)

add_custom_command(
  OUTPUT
    ${CMAKE_CURRENT_BINARY_DIR}/__header.h  # fake! ensure we run!
    # this command must generate: ${CMAKE_CURRENT_BINARY_DIR}/header.h
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_CURRENT_SOURCE_DIR}/content
    ${CMAKE_CURRENT_BINARY_DIR}/content
)

# --- Package ---

set(CPACK_PACKAGE_NAME "VoxelRenderer")
set(CPACK_OUTPUT_CONFIG_FILE "${CMAKE_BINARY_DIR}/CPackConfig-VoxelRenderer.cmake")
install(TARGETS VoxelRenderer)

include(CPack)
