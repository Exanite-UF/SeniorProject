#pragma once

#include <atomic>
#include <future>
#include <memory>
#include <optional>
#include <shared_mutex>

#include <src/gameobjects/Component.h>
#include <src/threading/PendingTasks.h>
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

    // Used primarily by the chunk manager
    struct ChunkManagerData
    {
        // Used by chunk modification threads
        PendingTasks<void> pendingTasks {};

        // Used by LODing system
        std::vector<std::shared_ptr<VoxelChunkData>> lods {};
        int activeLod = 0; // The actual LOD level. This is limited by the possible amount of LODs that can be generated.
        int requestedActiveLod = 0; // The active LOD level, as requested by the user. This may be higher than what is used.
        int requestedMaxLod = 0; // The max LOD level, as requested by the user. This may be higher than the number of LODs that can be generated.

        // For caching
        bool isPendingDestroy = false;
        bool isUploadDesired = false;
        int desiredLod = 0;
    };

private:
    std::optional<std::unique_ptr<VoxelChunk>> chunk; // Primarily accessed by render and chunk modification thread
    VoxelChunkData chunkData {}; // Primarily accessed by chunk modification thread

    RendererData rendererData {};
    ChunkManagerData chunkManagerData {};

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

    // This method itself is unsynchronized
    // See struct declaration for additional rules
    RendererData& getRendererData();

    // This method itself is unsynchronized
    // See struct declaration for additional rules
    ChunkManagerData& getChunkManagerData();

    bool getExistsOnGpu() const;

    std::pair<float, glm::vec3> raycast(glm::vec3 start, glm::vec3 direction, float currentDepth);

protected:
    void onRemovingFromWorld() override;

private:
    void allocateGpuData(const glm::ivec3& size);
    void deallocateGpuData();

    std::uint8_t getOccupancyByte(glm::ivec3 coord, int mipMapTexture);
};
