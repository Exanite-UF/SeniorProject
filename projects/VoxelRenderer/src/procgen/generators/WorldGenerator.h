#pragma once

#include <glm/vec3.hpp>

#include <memory>

#include <src/Constants.h>
#include <src/world/VoxelChunkComponent.h>
#include <src/world/VoxelChunkData.h>

class WorldGenerator : public NonCopyable
{
protected:
    glm::ivec3 chunkSize = Constants::VoxelChunkComponent::chunkSize;
    glm::ivec3 chunkPosition = glm::ivec3(0);

    virtual void generateData(VoxelChunkData& data) = 0;

public:
    explicit WorldGenerator();

    void generate(VoxelChunkData& data);
    void generate(VoxelChunkComponent& chunk); // Mainly for testing purposes

    virtual void showDebugMenu() = 0;

    void setChunkSize(const glm::ivec3& chunkSize);
    const glm::ivec3& getChunkSize();

    void setChunkPosition(const glm::ivec3& chunkPosition);
    const glm::ivec3& getChunkPosition();
};
