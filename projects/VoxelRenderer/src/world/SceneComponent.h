#pragma once

#include <glm/gtx/hash.hpp>

#include <memory>
#include <vector>

#include <src/gameobjects/Component.h>
#include <src/world/CameraComponent.h>
#include <src/world/VoxelChunkComponent.h>

class SceneComponent : public Component
{
    friend class VoxelChunkManager;

private:
    // The currently active camera
    std::shared_ptr<CameraComponent> camera {};

    // All chunks, both ones that are uploaded to the GPU and ones that only exist on the CPU
    std::vector<std::shared_ptr<VoxelChunkComponent>> chunks {};
    std::unordered_map<glm::ivec3, std::shared_ptr<VoxelChunkComponent>> chunksByChunkPosition {};

    // Chunks that should be rendered
    std::vector<std::shared_ptr<VoxelChunkComponent>> visibleChunks {};

    std::shared_mutex mutex {};

public:
    std::shared_mutex& getMutex();

    void setCamera(const std::shared_ptr<CameraComponent>& camera);
    const std::shared_ptr<CameraComponent>& getCamera();

    const std::vector<std::shared_ptr<VoxelChunkComponent>>& getChunks();
    const std::vector<std::shared_ptr<VoxelChunkComponent>>& getVisibleChunks();
    bool tryGetChunkAtPosition(const glm::ivec3& chunkPosition, std::shared_ptr<VoxelChunkComponent>& result);
    bool tryGetClosestChunk(std::shared_ptr<VoxelChunkComponent>& result);

    void addChunk(const glm::ivec3& chunkPosition, std::shared_ptr<VoxelChunkComponent>& chunk);
    void removeChunk(const glm::ivec3& chunkPosition);
};
