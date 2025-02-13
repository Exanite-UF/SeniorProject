# Voxel Renderer

## Getting started

1. Clone the repo.

2. Ensure Git submodules are up to date:

```sh
git submodule update --init --recursive
```

3. Open the root repo folder (or CMakeLists.txt) file as a project in your IDE or use CMake through the command line.

## Project organization

- /external - Contains 3rd party libraries, usually as submodules
- /projects - Contains 1st party projects
  - /VoxelRenderer - The main VoxelRenderer project
    - /assets - Source assets that are yet to be exported.
    - /content - Assets ready to be used by the game. Copied into the build output folder by CMake.
    - /src - Source code
      - /graphics - General-purpose OpenGL abstractions and other utilities. See /rendering for the rendering code.
      - /input - Input management code. Can be related to windowing, but usually higher level.
      - /rendering - Renderers and other specialized rendering code.
      - /utilities - Misc code that currently don't belong anywhere else.
      - /windowing - GLFW window management code.
      - /world - World state, world objects, scenes, transforms, etc.

## Packaging

This project uses CPack to package the built project. The project must be built first before packaging.

To package:
```sh
# Must be ran inside the generated CMake build directory
cmake --build .
cpack --config CPackConfig-VoxelRenderer.cmake
```
