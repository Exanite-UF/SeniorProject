#pragma once

#include <glm/gtx/hash.hpp>

#include <condition_variable>
#include <memory>
#include <queue>
#include <src/Constants.h>
#include <unordered_set>

#include <src/threading/PendingTasks.h>
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

        // The distance at which chunks begin to be loaded on a separate thread
        int loadDistance = 1;
        float lodBaseDistance = 256;
        float lodDistanceScalingFactor = 2.0f;

        // ----- Chunks -----

        // False prevents chunks from loading and unloading
        bool isChunkLoadingEnabled = true;

        // False prevents chunks from having their LODs generated and used
        bool isChunkLoddingEnabled = true;

        // False prevents chunks from having their CPU-side mipmaps generated
        bool areChunkCpuMipmapsEnabled = false; // TODO: Enable once CPU mipmap generation is fixed

        // Delay before a chunk marked for unloading is actually unloaded
        float chunkUnloadTime = 1;

        glm::ivec3 chunkSize = Constants::VoxelChunkComponent::chunkSize;

        // ---- Culling -----

        // Only works in DEBUG builds
        bool showDebugVisualizations = false;

        bool enableCulling = true;
    };

private:
    struct ChunkModificationTask
    {
    public:
        std::shared_ptr<VoxelChunkComponent> component {};
        std::shared_ptr<SceneComponent> scene {};
        VoxelChunkCommandBuffer commandBuffer {};

        std::promise<void> promise {};
        std::shared_future<void> future {};
        CancellationToken cancellationToken {};
        PendingTasks<void> dependencies;

        explicit ChunkModificationTask(const std::shared_ptr<VoxelChunkComponent>& component, const std::shared_ptr<SceneComponent>& scene, const VoxelChunkCommandBuffer& commandBuffer, const CancellationToken& cancellationToken);
    };

    struct ChunkLoadTask
    {
    public:
        glm::ivec2 chunkPosition;
        glm::ivec3 chunkSize;

        std::shared_ptr<VoxelChunkData> chunkData;

        explicit ChunkLoadTask(const glm::ivec2& chunkPosition, const glm::ivec3& chunkSize);
    };

    class ActiveWorldChunk : public NonCopyable
    {
    public:
        std::shared_ptr<VoxelChunkComponent> component {};
        glm::ivec2 chunkPosition {};

        bool isLoading = true;

        bool isUnloading = false; // TODO: This variable does the same thing as VoxelChunkComponent's isPendingDestroy
        float timeSpentWaitingForUnload = 0;

        std::shared_ptr<SceneComponent> scene {};

        explicit ActiveWorldChunk(const glm::ivec2& chunkPosition, const glm::ivec3& chunkSize, const std::shared_ptr<SceneComponent>& scene);
        ~ActiveWorldChunk() override;
    };

    // Helper class for tracking whether a thread has exited
    class TrackedThread : public NonCopyable
    {
    public:
        std::thread thread {};

    private:
        std::mutex mutex {};
        std::atomic<bool> isRunning = false;

    public:
        explicit TrackedThread(const std::function<void()>& threadEntrypoint);

        bool getIsRunning();
    };

    struct ManagerState
    {
    public:
        // ----- Primary state -----

        std::atomic<bool> isRunning = false;
        std::vector<std::shared_ptr<GlfwContext>> modificationThreadContexts {};

        // ----- Scene -----

        std::shared_ptr<SceneComponent> scene;
        std::unordered_map<glm::ivec2, std::unique_ptr<ActiveWorldChunk>> activeChunks {};

        // ----- Camera -----

        glm::vec3 cameraWorldPosition {};
        glm::ivec2 cameraChunkPosition {};
        glm::vec2 cameraFloatChunkPosition {};

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

        std::mutex pendingTasksMutex {};
        std::condition_variable pendingTasksCondition {};
        std::queue<std::shared_ptr<ChunkLoadTask>> pendingTasks {};

        std::mutex completedTasksMutex {};
        std::queue<std::shared_ptr<ChunkLoadTask>> completedTasks {};
    };

    struct ModificationThreadState
    {
    public:
        // The number of threads that can be actively working
        // The total number of threads can be higher
        std::atomic<int> concurrencyCount = 0;

        // The number of threads that are waiting for dependencies
        // This does not include threads limited by the activeSemaphore
        int waitingForDependencyCount = 0;

        std::unordered_set<std::shared_ptr<TrackedThread>> threads {};

        std::mutex threadManagementMutex {};
        std::shared_ptr<std::counting_semaphore<64>> activeSemaphore; // Used to limit the number of actively working threads

    public:
        std::recursive_mutex pendingTasksMutex {};
        std::condition_variable_any pendingTasksCondition {};
        std::deque<std::shared_ptr<ChunkModificationTask>> pendingTasks {}; // Deque is used to allow indexing, but pendingTasks is used as a queue

    public:
        // Allow only one thread to upload at a time
        // Used to limit the amount of uploads, mutual exclusion isn't required
        std::mutex gpuUploadMutex {};
    };

    class WaitingForDependenciesCounter
    {
    private:
        ModificationThreadState* state {};

    public:
        explicit WaitingForDependenciesCounter(ModificationThreadState* state);
        ~WaitingForDependenciesCounter();
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
    void initialize(const std::shared_ptr<SceneComponent>& scene, const std::vector<std::shared_ptr<GlfwContext>>& modificationThreadContexts);

    void update(float deltaTime);
    void showDebugMenu();

    // Can be called from any thread
    int getNotStartedCommandBufferCount();

    // Can be called from any thread
    std::shared_future<void> submitCommandBuffer(const std::shared_ptr<VoxelChunkComponent>& component, const VoxelChunkCommandBuffer& commandBuffer, const CancellationToken& cancellationToken = {});

    ~VoxelChunkManager() override;

protected:
    void onSingletonDestroy() override;

private:
    bool isChunkVisible(const std::shared_ptr<VoxelChunkComponent>& chunk, const std::shared_ptr<CameraComponent>& camera) const;

    // Must manually synchronize
    void cleanupCancelledCommandBuffers();

    // Must manually synchronize
    void createModificationThread();
};
