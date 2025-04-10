# Voxel Renderer

## Getting started

1. Clone the repo.

2. Ensure Git submodules are up to date:

```sh
git submodule update --init --recursive
```

3. Open the root repo folder (or CMakeLists.txt) file as a project in your IDE or use CMake through the command line.
   - Optionally, use your IDE's CMakePresets integration to select a preset. We support MSVC on Windows and GCC on Linux.

## Project organization

- /external - Contains 3rd party libraries, usually as submodules
- /projects - Contains 1st party projects
  - /VoxelRenderer - The main VoxelRenderer project
    - /assets - Source assets that are yet to be exported.
    - /content - Assets ready to be used by the game. Copied into the build output folder by CMake.
    - /src - Source code
      - /gameobjects - Entity-Component (EC) GameObject and scene graph implementation.
      - /graphics - General-purpose OpenGL abstractions and other utilities. See /rendering for the rendering code.
      - /input - Input management code. Can be related to windowing, but usually higher level.
      - /procgen - Code related to generating voxel worlds.
      - /rendering - Renderers and other specialized rendering code.
      - /threading - Code used for threading.
      - /utilities - Misc code that currently don't belong anywhere else.
      - /voxelizer - Code for converting meshes into voxel prefabs.
      - /windowing - GLFW window management code.
      - /world - World state, world objects, scenes, transforms, etc.

## Packaging

This project uses CPack to package the built project. The project must be built first before packaging.

To package:
```sh
# Must be ran inside the generated CMake build directory
cmake --build . --target VoxelRenderer
cpack -P VoxelRenderer -DCPACK_COMPONENTS_ALL="VoxelRenderer" -G ZIP
```
