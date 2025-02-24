#pragma once

#include <glm/vec3.hpp>
#include <memory>
#include <src/world/VoxelWorldData.h>

class WorldGenerator : public NonCopyable
{
protected:
    VoxelWorldData data;

    virtual void generateData() = 0;

public:
    explicit WorldGenerator(glm::ivec3 worldSize);
    void generate(VoxelWorld& voxelWorld);
    virtual void showDebugMenu() = 0;
};
