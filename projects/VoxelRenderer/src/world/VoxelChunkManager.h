#pragma once

#include <glm/gtx/hash.hpp>

#include <condition_variable>
#include <memory>
#include <queue>
#include <src/Constants.h>

#include <src/utilities/Singleton.h>
#include <src/windowing/GlfwContext.h>
#include <src/world/SceneComponent.h>
#include <src/world/VoxelChunkCommandBuffer.h>
#include <src/world/VoxelChunkData.h>

class VoxelChunkManager : public Singleton<VoxelChunkManager>
{
public:
    struct ManagerSettings
    {
    public:
        // ----- Rendering -----

        // The distance at which chunks begin to be generated on a separate thread
        int generationDistance = 2; // TODO

        // The distance at which chunks are loaded and uploaded to the GPU
        int renderDistance = 1; // TODO: Increase renderDistance to 2 after LODs are added

        // ----- Chunks -----

        // False prevents chunks from loading and unloading
        bool isChunkLoadingEnabled = true;

        // Delay before a chunk marked for unloading is actually unloaded
        float chunkUnloadTime = 0; // TODO: This should be 1 after LODs are added

        glm::ivec3 chunkSize = Constants::VoxelChunkComponent::chunkSize;
    };

private:
    struct ChunkModificationTask
    {
    public:
        std::shared_ptr<VoxelChunkComponent> component;
        VoxelChunkCommandBuffer commandBuffer;

        explicit ChunkModificationTask(const std::shared_ptr<VoxelChunkComponent>& component, const VoxelChunkCommandBuffer& commandBuffer);
    };

    struct ChunkLoadTask
    {
    public:
        glm::ivec2 chunkPosition;
        glm::ivec3 chunkSize;

        std::shared_ptr<VoxelChunkData> chunkData;

        explicit ChunkLoadTask(const glm::ivec2& chunkPosition, const glm::ivec3& chunkSize);
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

    struct ManagerState
    {
    public:
        // ----- Primary state -----

        std::atomic<bool> isRunning = false;
        std::shared_ptr<GlfwContext> modificationThreadContext;

        // ----- Scene -----

        std::shared_ptr<SceneComponent> scene;
        std::unordered_map<glm::ivec2, std::unique_ptr<ActiveChunk>> activeChunks {};

        // ----- Camera -----

        glm::vec3 cameraWorldPosition {};
        glm::ivec2 cameraChunkPosition {};

        // ----- Chunk loading -----

        // If true, then we need to check for chunks to load/unload and mark them as such. We will load/unload them in a separate step
        bool isChunkLoadingDirty = true;

        // If true, then we need to check for chunks to unload
        bool isChunkUnloadingDirty = true;
    };

    struct LoadingThreadState
    {
    public:
        std::vector<std::thread> threads {};

        std::condition_variable pendingTasksCondition {};
        std::mutex pendingTasksMutex {};
        std::queue<std::shared_ptr<ChunkLoadTask>> pendingTasks {};

        std::mutex completedTasksMutex {};
        std::queue<std::shared_ptr<ChunkLoadTask>> completedTasks {};
    };

    struct ModificationThreadState
    {
    public:
        std::vector<std::thread> threads {};

        std::condition_variable_any pendingTasksCondition {};
        std::recursive_mutex pendingTasksMutex {};
        std::queue<std::shared_ptr<ChunkModificationTask>> pendingTasks {};
    };

public:
    ManagerSettings settings {};

private:
    ManagerState state {};
    LoadingThreadState loadingThreadState {};
    ModificationThreadState modificationThreadState {};

    void chunkLoadingThreadEntrypoint(int threadId);
    void chunkModificationThreadEntrypoint(int threadId);

public:
    void initialize(const std::shared_ptr<SceneComponent>& scene, const std::shared_ptr<GlfwContext>& modificationThreadContext);

    void update(float deltaTime);
    void showDebugMenu();

    // Can be called from any thread
    void submitCommandBuffer(const std::shared_ptr<VoxelChunkComponent>& component, const VoxelChunkCommandBuffer& commandBuffer);

    ~VoxelChunkManager() override;

protected:
    void onSingletonDestroy() override;
};
