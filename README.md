# Voxel Renderer

## Getting started

1. Clone the repo.

2. Ensure Git submodules are up to date:

```sh
git submodule update --init --recursive
```

3. Open the root repo folder (or CMakeLists.txt) file as a project in your IDE or use CMake through the command line.

## Packaging

This project uses CPack to package the built project. The project must be built first before packaging.

To package:
```sh
# Must be ran inside the generated CMake build directory
cmake --build .
cpack --config CPackConfig-VoxelRenderer.cmake
```
