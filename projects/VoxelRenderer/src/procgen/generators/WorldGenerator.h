#pragma once

#include <glm/vec3.hpp>
#include <memory>
#include <src/world/VoxelChunkData.h>

class WorldGenerator : public NonCopyable
{
protected:
    VoxelChunkData data;

    virtual void generateData() = 0;

public:
    explicit WorldGenerator(glm::ivec3 chunkSize);

    void generate(VoxelChunk& chunk);
    virtual void showDebugMenu() = 0;
};
