#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>

#include <src/gameobjects/Component.h>
#include <src/world/VoxelChunk.h>
#include <src/world/VoxelChunkData.h>

class VoxelChunkComponent : public Component
{
    friend class GameObject;

private:
    std::optional<std::unique_ptr<VoxelChunk>> chunk; // Primarily accessed by render and chunk modification thread
    VoxelChunkData chunkData {}; // Primarily accessed by chunk modification thread

    std::atomic<bool> existsOnGpu = false;

    std::shared_mutex mutex {};

public:
    explicit VoxelChunkComponent();
    explicit VoxelChunkComponent(bool shouldGeneratePlaceholderData);

    // shared access must be acquired when calling getChunk()
    // exclusive access must be acquired when calling any non-const method
    std::shared_mutex& getMutex();

    // Will throw if chunk data does not exist on the GPU
    const std::unique_ptr<VoxelChunk>& getChunk();
    VoxelChunkData& getChunkData();

    bool getExistsOnGpu() const;
    void setExistsOnGpu(bool existsOnGpu);

    void onDestroy() override;
};
