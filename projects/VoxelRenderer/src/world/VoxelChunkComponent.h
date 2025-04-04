#pragma once

#include <atomic>
#include <condition_variable>
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

public:
    // Used primarily by the renderer
    struct RendererData
    {
        std::atomic<bool> isVisible = false;

        // Used to calculate motion vectors
        glm::vec3 previousPosition;
        glm::quat previousRotation;
        glm::vec3 previousScale;
    };

    // Used primarily by the chunk modification threads
    struct ModificationData
    {
        // The current version of the chunk data
        // If this equals pending, then the chunk data is up to date
        // Overflows of the version values are fine
        int version = 0;

        // The pending version of the chunk
        // This is based on the number of pending VoxelChunkCommandBuffer submitted
        int pendingVersion = 0;

        // For synchronization
        // Chunk modification threads should wait until the version is equal to the expected version to make sure that command buffers are applied in order
        // After modification, chunk modification threads should increment the version by 1
        // Try to avoid locking this mutex for a long time
        std::mutex versionMutex {};
        std::condition_variable versionCondition {};
    };

private:
    std::optional<std::unique_ptr<VoxelChunk>> chunk; // Primarily accessed by render and chunk modification thread
    VoxelChunkData chunkData {}; // Primarily accessed by chunk modification thread

    std::atomic<bool> existsOnGpu = false;
    RendererData rendererData {};
    ModificationData modificationData {};

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

    // Unsynchronized
    RendererData& getRendererData();

    // This method itself is unsynchronized
    // See struct declaration for additional rules
    ModificationData& getModificationData();

    bool getExistsOnGpu() const;

protected:
    void onRemovingFromWorld() override;

private:
    void setExistsOnGpu(bool existsOnGpu, bool writeToGpu = true);
};
