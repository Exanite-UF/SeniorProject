#pragma once

#include <memory>
#include <optional>
#include <src/gameobjects/Component.h>
#include <src/world/VoxelChunk.h>
#include <src/world/VoxelChunkData.h>

class VoxelChunkComponent : public Component
{
    friend class GameObject;

private:
    std::optional<std::unique_ptr<VoxelChunk>> chunk;
    VoxelChunkData chunkData {};

    bool existsOnGpu = false;

public:
    explicit VoxelChunkComponent();
    explicit VoxelChunkComponent(bool shouldGeneratePlaceholderData);

    // Will throw if chunk data does not exist on the GPU
    const std::unique_ptr<VoxelChunk>& getChunk(); // TODO: Add mutex for chunk and another one for chunkData
    VoxelChunkData& getChunkData();

    bool getExistsOnGpu() const;
    void setExistsOnGpu(bool existsOnGpu);

    void onDestroy() override;
};
