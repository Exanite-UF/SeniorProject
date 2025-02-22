#pragma once

#include <memory>
#include <src/world/VoxelWorldData.h>
#include <glm/vec3.hpp>

class WorldGenerator
{
private:
    VoxelWorldData data;

    float seed = 0;
    float baseHeight = 100;
    int octaves = 3;
    float persistence = 0.5;

public:
    explicit WorldGenerator(glm::ivec3 worldSize);
    void generate(VoxelWorld& voxelWorld);
    virtual void generate();
    virtual void showDebugMenu();
};
