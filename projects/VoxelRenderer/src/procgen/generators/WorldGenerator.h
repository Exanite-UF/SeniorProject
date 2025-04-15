#pragma once

#include <glm/vec3.hpp>

#include <memory>

#include <src/Constants.h>
#include <src/world/VoxelChunkComponent.h>
#include <src/world/VoxelChunkData.h>
#include <src/world/SceneComponent.h>

class WorldGenerator : public NonCopyable
{
protected:
    glm::ivec3 chunkSize = Constants::VoxelChunkComponent::chunkSize;
    glm::ivec3 chunkPosition = glm::ivec3(0);
    std::shared_ptr<SceneComponent> scene = nullptr;

    virtual void generateData(VoxelChunkData& data) = 0;

public:
    explicit WorldGenerator();

    // Most generators expect the provided VoxelChunkData to be already cleared
    // clearData is mainly used to avoid clearing a cleared VoxelChunkData
    void generate(VoxelChunkData& data, std::shared_ptr<SceneComponent> scene, bool clearData = true);
    void generate(const std::shared_ptr<VoxelChunkComponent>& chunk,  std::shared_ptr<SceneComponent> scene); // Mainly for testing purposes

    virtual void showDebugMenu() = 0;

    void setChunkSize(const glm::ivec3& chunkSize);
    const glm::ivec3& getChunkSize() const;

    void setChunkPosition(const glm::ivec3& chunkPosition);
    const glm::ivec3& getChunkPosition() const;
};
