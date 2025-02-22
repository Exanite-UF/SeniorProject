#pragma once

#include <memory>
#include <src/world/VoxelWorldData.h>
#include <glm/vec3.hpp>

class WorldGenerator
{
protected:
    VoxelWorldData data;

    virtual void generateData() = 0;

public:
    explicit WorldGenerator(glm::ivec3 worldSize);
    void generate(VoxelWorld& voxelWorld);
    virtual void showDebugMenu() = 0;
};
