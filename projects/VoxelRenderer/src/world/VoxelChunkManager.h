#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <condition_variable>
#include <memory>
#include <queue>

#include <src/utilities/Log.h>
#include <src/utilities/Singleton.h>
#include <src/world/SceneComponent.h>
#include <src/world/VoxelChunkData.h>

class VoxelChunkManager : public Singleton<VoxelChunkManager>
{
private:
    struct ChunkLoadRequest
    {
    public:
        glm::ivec2 chunkPosition;
        glm::ivec3 chunkSize;

        VoxelChunkData chunkData;

        explicit ChunkLoadRequest(const glm::ivec2& chunkPosition, const glm::ivec3& chunkSize)
        {
            this->chunkPosition = chunkPosition;
            this->chunkSize = chunkSize;
        }
    };

    struct LoadedChunkData
    {
    public:
        std::shared_ptr<VoxelChunkComponent> chunk {};
        glm::ivec2 chunkPosition;

        bool isLoading = true;

        bool isUnloading = false;
        float unloadWaitTime = 0;

        explicit LoadedChunkData(const glm::ivec2& chunkPosition)
        {
            this->chunkPosition = chunkPosition;
        }
    };

    struct ManagerData
    {
    public:
        // ----- Rendering -----

        // The distance at which chunks begin to be generated on a separate thread
        int generationDistance = 3; // TODO

        // The distance at which chunks are loaded and uploaded to the GPU
        int renderDistance = 2;

        // ----- Loading -----

        int chunkLoadingThreadCount = 0;
        std::vector<std::thread> chunkLoadingThreads {};

        // Used to wake up the chunk loading thread
        std::condition_variable chunkLoadingThreadCondition {};

        std::mutex pendingRequestsMutex {};
        std::mutex completedRequestsMutex {};

        std::queue<std::shared_ptr<ChunkLoadRequest>> pendingRequests {};
        std::queue<std::shared_ptr<ChunkLoadRequest>> completedRequests {};

        // ----- Unloading -----

        // Delay before a chunk marked for unloading is actually unloaded
        float chunkUnloadTime = 1;

        // ----- Camera -----

        glm::vec3 cameraWorldPosition {};
        glm::ivec2 cameraChunkPosition {};

        // ----- Caching -----

        // If true, then we need to check for chunks to load/unload and *mark* them as such. We will load/unload them in a separate step
        bool isChunkLoadingDirty = true;

        // If true, then we need to check for chunks to unload
        bool isChunkUnloadingDirty = true;

        // ----- Chunks -----

        std::unordered_map<glm::ivec2, LoadedChunkData> loadedChunks {};
    };

private:
    std::atomic<bool> isRunning = false;
    std::shared_ptr<SceneComponent> scene;

    ManagerData data;

    void chunkLoaderThreadEntrypoint();

public:
    void initialize(const std::shared_ptr<SceneComponent>& scene);

    void update(float deltaTime);
    void showDebugMenu();

    ~VoxelChunkManager() override;
};
