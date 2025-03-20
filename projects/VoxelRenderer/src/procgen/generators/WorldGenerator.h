#pragma once

#include <glm/vec3.hpp>
#include <memory>
#include <src/world/VoxelChunkData.h>

class WorldGenerator : public NonCopyable
{
protected:
    virtual void generateData(VoxelChunkData& data) = 0;

public:
    explicit WorldGenerator();

    void generate(VoxelChunkData& data);
    void generate(VoxelChunk& chunk); // TODO: Remove this overload

    virtual void showDebugMenu() = 0;
};
