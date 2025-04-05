#pragma once

#include <glm/gtx/hash.hpp>

#include <memory>
#include <vector>

#include <src/gameobjects/Component.h>
#include <src/world/CameraComponent.h>
#include <src/world/SkyboxComponent.h>
#include <src/world/VoxelChunkComponent.h>

class SceneComponent : public Component
{
    friend class VoxelChunkManager;
    friend class VoxelChunkCommandBuffer;

private:
    // The currently active camera
    std::shared_ptr<CameraComponent> camera {};

    // All chunks, both ones that are uploaded to the GPU and ones that only exist on the CPU
    // This includes both world chunks and object chunks
    std::vector<std::shared_ptr<VoxelChunkComponent>> allChunks {};

    // Object chunks are other VoxelChunks that represent (usually) smaller objects in the world
    std::vector<std::shared_ptr<VoxelChunkComponent>> objectChunks {};

    // World chunks are managed by the world generator and represent large sections of the world
    std::vector<std::shared_ptr<VoxelChunkComponent>> worldChunks {};

    // Speeds up searching for world chunks by chunk position
    std::unordered_map<glm::ivec3, std::shared_ptr<VoxelChunkComponent>> worldChunksByChunkPosition {};

    std::shared_ptr<SkyboxComponent> skybox; // = std::make_shared<SkyboxComponent>();//A default void

    // General use mutex
    // Readers should acquire shared access
    // Writers should acquire exclusive access
    std::shared_mutex mutex {};

public:
    std::shared_mutex& getMutex();

    void setCamera(const std::shared_ptr<CameraComponent>& camera);
    const std::shared_ptr<CameraComponent>& getCamera();

    const std::vector<std::shared_ptr<VoxelChunkComponent>>& getAllChunks();
    const std::vector<std::shared_ptr<VoxelChunkComponent>>& getObjectChunks();
    const std::vector<std::shared_ptr<VoxelChunkComponent>>& getWorldChunks();

    bool tryGetWorldChunkAtPosition(const glm::ivec3& chunkPosition, std::shared_ptr<VoxelChunkComponent>& outResult);
    bool tryGetClosestWorldChunk(std::shared_ptr<VoxelChunkComponent>& outResult);

    void addWorldChunk(const glm::ivec3& chunkPosition, std::shared_ptr<VoxelChunkComponent>& chunk);
    void removeWorldChunk(const glm::ivec3& chunkPosition);

    void setSkybox(std::shared_ptr<SkyboxComponent>& skybox);
    std::shared_ptr<SkyboxComponent> getSkybox();
};
