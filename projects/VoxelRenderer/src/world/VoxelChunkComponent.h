#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <optional>
#include <shared_mutex>

#include <src/gameobjects/Component.h>
#include <src/world/VoxelChunk.h>
#include <src/world/VoxelChunkData.h>

class VoxelChunkCommandBuffer;

class VoxelChunkComponent : public Component
{
    friend class GameObject;
    friend class VoxelChunkCommandBuffer;

private:
    std::optional<std::unique_ptr<VoxelChunk>> chunk; // Primarily accessed by render and chunk modification thread
    VoxelChunkData chunkData {}; // Primarily accessed by chunk modification thread

    std::atomic<bool> existsOnGpu = false;

    std::shared_mutex mutex {};

public:
    explicit VoxelChunkComponent();
    explicit VoxelChunkComponent(bool shouldGeneratePlaceholderData);

    std::shared_mutex& getMutex();

    // Will throw if chunk data does not exist on the GPU
    // Requires mutex shared access
    const std::unique_ptr<VoxelChunk>& getChunk();

    // Requires mutex shared access
    const VoxelChunkData& getChunkData();

    // Requires mutex exclusive access
    // Prefer using a command buffer instead
    VoxelChunkData& getRawChunkData();

    bool getExistsOnGpu() const;

protected:
    void onDestroy() override;

private:
    void setExistsOnGpu(bool existsOnGpu, bool writeToGpu = true);
};
