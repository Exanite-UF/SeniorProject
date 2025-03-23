#pragma once

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <condition_variable>
#include <memory>
#include <queue>
#include <src/Constants.h>

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

        explicit ChunkLoadRequest(const glm::ivec2& chunkPosition, const glm::ivec3& chunkSize);
    };

    class ActiveChunk : public NonCopyable
    {
    public:
        std::shared_ptr<VoxelChunkComponent> component {};
        glm::ivec2 chunkPosition;

        bool isLoading = true;

        bool isUnloading = false;
        float timeSpentWaitingForUnload = 0;

        bool isDisplayed = false;
        std::shared_ptr<SceneComponent> scene;

        explicit ActiveChunk(const glm::ivec2& chunkPosition, const glm::ivec3& chunkSize, const std::shared_ptr<SceneComponent>& scene);
        ~ActiveChunk() override;
    };

    struct ManagerData
    {
    public:
        // ----- Primary state -----

        std::atomic<bool> isRunning = false;

        // ----- Scene -----

        std::shared_ptr<SceneComponent> scene;

        // ----- Camera -----

        glm::vec3 cameraWorldPosition {};
        glm::ivec2 cameraChunkPosition {};

        // ----- Chunks -----

        glm::ivec3 chunkSize = Constants::VoxelChunkComponent::chunkSize;
        std::unordered_map<glm::ivec2, std::unique_ptr<ActiveChunk>> activeChunks {};

        // ----- Rendering -----

        // The distance at which chunks begin to be generated on a separate thread
        int generationDistance = 2; // TODO

        // The distance at which chunks are loaded and uploaded to the GPU
        int renderDistance = 1; // TODO: Increase renderDistance to 2 after LODs are added

        // ----- Chunk loading -----

        bool isChunkLoadingEnabled = true;

        // If true, then we need to check for chunks to load/unload and mark them as such. We will load/unload them in a separate step
        bool isChunkLoadingDirty = true;

        // If true, then we need to check for chunks to unload
        bool isChunkUnloadingDirty = true;

        // ----- Chunk unloading -----

        // Delay before a chunk marked for unloading is actually unloaded
        float chunkUnloadTime = 0; // TODO: This should be 1 after LODs are added

        // ----- Chunk loading threads -----

        std::vector<std::thread> chunkLoadingThreads {};

        // Used to wake up the chunk loading thread
        std::condition_variable chunkLoadingThreadCondition {};

        std::mutex pendingRequestsMutex {};
        std::mutex completedRequestsMutex {};

        std::queue<std::shared_ptr<ChunkLoadRequest>> pendingRequests {};
        std::queue<std::shared_ptr<ChunkLoadRequest>> completedRequests {};
    };

private:
    ManagerData data {};

    void chunkLoaderThreadEntrypoint();

public:
    void initialize(const std::shared_ptr<SceneComponent>& scene);

    void update(float deltaTime);
    void showDebugMenu();

    ~VoxelChunkManager() override;

protected:
    void onSingletonDestroy() override;
};
