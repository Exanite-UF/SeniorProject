#pragma once

#include <memory>
#include <src/gameobjects/Component.h>
#include <src/world/VoxelChunk.h>
#include <src/world/VoxelChunkData.h>
#include <optional>

class VoxelChunkComponent : public Component
{
    friend class GameObject;

private:
    std::optional<std::unique_ptr<VoxelChunk>> chunk;
    VoxelChunkData chunkData {};

    bool existsOnGpu = false;

public:
    explicit VoxelChunkComponent();
    explicit VoxelChunkComponent(bool generatePlaceholderData);

    // Will throw if chunk data does not exist on the GPU
    const std::unique_ptr<VoxelChunk>& getChunk();
    VoxelChunkData& getChunkData();

    bool getExistsOnGpu() const;
    void setExistsOnGpu(bool existsOnGpu);
};
