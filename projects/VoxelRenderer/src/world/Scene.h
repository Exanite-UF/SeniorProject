#pragma once

#include <vector>

#include <src/world/Camera.h>
#include <src/world/VoxelWorld.h>

class Scene
{
public:
    // TODO: Implement transformation hierarchy

    std::vector<VoxelWorld> worlds;
    Camera camera;
};
