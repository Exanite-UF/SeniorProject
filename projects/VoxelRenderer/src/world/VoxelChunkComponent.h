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

    std::shared_mutex chunkMutex {};
    std::mutex chunkDataMutex {};

public:
    explicit VoxelChunkComponent();
    explicit VoxelChunkComponent(bool shouldGeneratePlaceholderData);

    // Will throw if chunk data does not exist on the GPU
    // Caller needs to acquire the corresponding mutex for thread safety
    const std::unique_ptr<VoxelChunk>& getChunkUnsafe();
    std::shared_mutex& getChunkMutex();

    // Caller needs to acquire the corresponding mutex for thread safety
    VoxelChunkData& getChunkDataUnsafe();
    std::mutex& getChunkDataMutex();

    bool getExistsOnGpu() const;
    void setExistsOnGpu(bool existsOnGpu);

    void onDestroy() override;
};
