#pragma once

#include <memory>
#include <src/gameobjects/Component.h>
#include <src/world/VoxelChunk.h>
#include <src/world/VoxelChunkData.h>

class VoxelChunkComponent : public Component
{
    friend class GameObject;

public:
    // TODO: Better encapsulation
    std::optional<std::unique_ptr<VoxelChunk>> chunk;
    VoxelChunkData chunkData {};

    bool isDisplayed = false;

    explicit VoxelChunkComponent();
    explicit VoxelChunkComponent(bool generatePlaceholderData);

    // Will throw if chunk data does not exist on the GPU
    // If the chunk is being displayed, this will always succeed
    const std::unique_ptr<VoxelChunk>& getChunk();

    bool getIsDisplayed() const;
    void setIsDisplayed(bool isDisplayed);
};
