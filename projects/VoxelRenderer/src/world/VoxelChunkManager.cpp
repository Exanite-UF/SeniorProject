#include "VoxelChunkManager.h"

#include <tracy/Tracy.hpp>

#include <algorithm>
#include <condition_variable>
#include <format>
#include <memory>
#include <queue>
#include <ranges>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <src/gameobjects/GameObject.h>
#include <src/procgen/generators/PrototypeWorldGenerator.h>
#include <src/procgen/synthesizers/TextureOctaveNoiseSynthesizer.h>
#include <src/utilities/Assert.h>
#include <src/utilities/ColorUtility.h>
#include <src/utilities/ImGui.h>
#include <src/utilities/ImGuiUtility.h>
#include <src/utilities/Log.h>
#include <src/utilities/MeasureElapsedTimeScope.h>
#include <src/world/VoxelChunkData.h>

VoxelChunkManager::ChunkModificationTask::ChunkModificationTask(
    const std::shared_ptr<VoxelChunkComponent>& component,
    const std::shared_ptr<SceneComponent>& scene,
    const VoxelChunkCommandBuffer& commandBuffer)
{
    ZoneScoped;

    this->component = component;
    this->scene = scene;
    this->commandBuffer = commandBuffer;
}

VoxelChunkManager::ChunkLoadTask::ChunkLoadTask(const glm::ivec2& chunkPosition, const glm::ivec3& chunkSize)
{
    ZoneScoped;

    this->chunkPosition = chunkPosition;
    this->chunkSize = chunkSize;
}

VoxelChunkManager::ActiveWorldChunk::ActiveWorldChunk(
    const glm::ivec2& chunkPosition,
    const glm::ivec3& chunkSize,
    const std::shared_ptr<SceneComponent>& scene)
{
    ZoneScoped;

    this->chunkPosition = chunkPosition;
    this->scene = scene;

    auto go = scene->getGameObject()->createChildObject(std::format("Chunk ({}, {})", chunkPosition.x, chunkPosition.y));
    component = go->addComponent<VoxelChunkComponent>();

    component->getTransform()->setGlobalPosition(glm::vec3(chunkSize.x * chunkPosition.x, chunkSize.y * chunkPosition.y, 0) + (glm::vec3(chunkSize) / 2.0f));

    scene->addWorldChunk(glm::ivec3(chunkPosition.x, chunkPosition.y, 0), component);
}

VoxelChunkManager::ActiveWorldChunk::~ActiveWorldChunk()
{
    ZoneScoped;

    std::lock_guard lock(scene->getMutex());

    scene->removeWorldChunk(glm::ivec3(chunkPosition.x, chunkPosition.y, 0));
    if (component->getIsPartOfWorld())
    {
        component->getGameObject()->removeFromWorld();
    }
}

VoxelChunkManager::~VoxelChunkManager() = default;

void VoxelChunkManager::onSingletonDestroy()
{
    Singleton::onSingletonDestroy();

    Log::information("Cleaning up VoxelChunkManager");

    state.isRunning = false;

    Log::information("Stopping VoxelChunkManager chunk loading threads");

    {
        std::lock_guard lock(loadingThreadState.pendingTasksMutex);
        loadingThreadState.pendingTasksCondition.notify_all();
    }

    Log::information("Stopping VoxelChunkManager chunk modification threads");

    {
        std::lock_guard lock(modificationThreadState.pendingTasksMutex);
        modificationThreadState.pendingTasksCondition.notify_all();
    }

    for (auto& thread : loadingThreadState.threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    for (auto& thread : modificationThreadState.threads)
    {
        if (thread.joinable())
        {
            thread.join();
        }
    }

    Log::information("Successfully cleaned up VoxelChunkManager");
}

void VoxelChunkManager::initialize(const std::shared_ptr<SceneComponent>& scene, const std::vector<std::shared_ptr<GlfwContext>>& modificationThreadContexts)
{
    Assert::isTrue(!state.isRunning, "VoxelChunkManager has already been initialized");

    state.modificationThreadContexts = modificationThreadContexts;

    state.isRunning = true;
    this->state.scene = scene;

    Log::information("Initializing VoxelChunkManager");

    // Create chunk loading threads
    {
        auto loadingThreadCount = std::max(1u, std::thread::hardware_concurrency() / 2);
        Log::information(std::format("Starting VoxelChunkManager {} chunk loading threads", loadingThreadCount));
        for (int i = 0; i < loadingThreadCount; ++i)
        {
            loadingThreadState.threads.push_back(std::thread(&VoxelChunkManager::chunkLoadingThreadEntrypoint, this, loadingThreadState.threads.size()));
        }
    }

    // Create chunk modification threads
    {
        auto modificationThreadCount = Constants::VoxelChunkManager::maxChunkModificationThreads; // Note: Each thread must have its own GlfwContext.
        Log::information(std::format("Starting VoxelChunkManager {} chunk modification threads", modificationThreadCount));
        for (int i = 0; i < modificationThreadCount; ++i)
        {
            modificationThreadState.threads.push_back(std::thread(&VoxelChunkManager::chunkModificationThreadEntrypoint, this, modificationThreadState.threads.size()));
        }
    }

    Log::information("Initialized VoxelChunkManager");
}

void VoxelChunkManager::chunkLoadingThreadEntrypoint(const int threadId)
{
    tracy::SetThreadName(std::format("Chunk loading {}", threadId).c_str());

    Log::debug("Started VoxelChunkManager chunk loading thread");

    while (state.isRunning)
    {
        // Create pending requests lock, but don't lock the mutex
        // Locking is done by condition variable below
        std::unique_lock pendingRequestsLock(loadingThreadState.pendingTasksMutex);

        // Wait for new requests to come in
        loadingThreadState.pendingTasksCondition.wait(pendingRequestsLock, [&]()
            {
                return !loadingThreadState.pendingTasks.empty() || !state.isRunning;
            });

        if (loadingThreadState.pendingTasks.empty() || !state.isRunning)
        {
            continue;
        }

        ZoneScopedN("Chunk loading task");

        // Take some work
        auto task = loadingThreadState.pendingTasks.front();
        loadingThreadState.pendingTasks.pop();

        // Unlock the mutex
        pendingRequestsLock.unlock();

        // Generate chunk
        task->chunkData = std::make_shared<VoxelChunkData>(task->chunkSize);
        Log::information(std::format("Generating chunk at ({}, {})", task->chunkPosition.x, task->chunkPosition.y));
        {
            ZoneScopedN("Chunk generation");

            MeasureElapsedTimeScope scope(std::format("Chunk generation for chunk at ({}, {})", task->chunkPosition.x, task->chunkPosition.y));

            int seed = 0;
            int octaves = 3;
            float persistence = 0.5;
            auto octaveSynthesizer = std::make_shared<TextureOctaveNoiseSynthesizer>(seed, octaves, persistence);

            PrototypeWorldGenerator generator(octaveSynthesizer);
            generator.setChunkSize(task->chunkSize);
            generator.setChunkPosition(glm::ivec3(task->chunkPosition, 0));

            generator.generate(*task->chunkData);
        }

        Log::verbose(std::format("Generated chunk at ({}, {})", task->chunkPosition.x, task->chunkPosition.y));

        // Acquire completed requests mutex
        std::unique_lock completedRequestsLock(loadingThreadState.completedTasksMutex);

        // Publish completed request
        loadingThreadState.completedTasks.push(task);
    }

    Log::debug("Stopped VoxelChunkManager chunk loading thread");
}

void VoxelChunkManager::chunkModificationThreadEntrypoint(const int threadId)
{
    tracy::SetThreadName(std::format("Chunk modification {}", threadId).c_str());

    Log::debug("Started VoxelChunkManager chunk modification thread");

    state.modificationThreadContexts.at(threadId)->makeContextCurrent();

    while (state.isRunning)
    {
        // Create pending requests lock, but don't lock the mutex
        // Locking is done by condition variable below
        std::unique_lock pendingRequestsLock(modificationThreadState.pendingTasksMutex);

        // Wait for new requests to come in
        modificationThreadState.pendingTasksCondition.wait(pendingRequestsLock, [&]()
            {
                return !modificationThreadState.pendingTasks.empty() || !state.isRunning;
            });

        if (modificationThreadState.pendingTasks.empty() || !state.isRunning)
        {
            continue;
        }

        ZoneScopedN("Chunk modification task");

        // Take some work
        auto task = modificationThreadState.pendingTasks.front();
        modificationThreadState.pendingTasks.pop();

        // Unlock the mutex
        pendingRequestsLock.unlock();

        // Apply the chunk command buffer
        {
            ZoneScopedN("Chunk modification");

            MeasureElapsedTimeScope scope(std::format("Apply chunk command buffer"), Log::Verbose);
            Log::verbose("Applying chunk command buffer");
            task->commandBuffer.apply(task->component, task->scene, modificationThreadState.gpuUploadMutex);
        }
    }

    Log::debug("Stopped VoxelChunkManager chunk modification thread");
}

void VoxelChunkManager::update(const float deltaTime)
{
    ZoneScoped;

    // Calculate new camera chunk position
    state.cameraWorldPosition = state.scene->camera->getTransform()->getGlobalPosition();

    auto newCameraChunkPosition = glm::ivec2(glm::round(glm::vec2(
        state.cameraWorldPosition.x / settings.chunkSize.x - 0.5f,
        state.cameraWorldPosition.y / settings.chunkSize.y - 0.5f)));

    if (state.cameraChunkPosition != newCameraChunkPosition)
    {
        // Camera chunk position has changed, we may need to load new chunks
        state.isChunkLoadingDirty = true;
    }

    state.cameraChunkPosition = newCameraChunkPosition;

    // Chunk loading logic
    if (state.isChunkLoadingDirty && settings.isChunkLoadingEnabled)
    {
        ZoneScopedN("Chunk loading logic");

        // Calculate which chunks should be loaded
        std::unordered_set<glm::ivec2> chunksToLoad {};
        for (int x = -settings.renderDistance; x <= settings.renderDistance; ++x)
        {
            for (int y = -settings.renderDistance; y <= settings.renderDistance; ++y)
            {
                auto chunkPosition = state.cameraChunkPosition + glm::ivec2(x, y);
                chunksToLoad.emplace(chunkPosition);
            }
        }

        // Unload chunks
        for (auto& loadedChunk : std::views::values(state.activeChunks))
        {
            if (loadedChunk->isUnloading)
            {
                // Chunk is already unloading, skip it
                continue;
            }

            if (chunksToLoad.contains(loadedChunk->chunkPosition))
            {
                // Chunk should be loaded, don't unload it
                continue;
            }

            // Mark the chunk to be unloaded
            loadedChunk->isUnloading = true;
            state.isChunkUnloadingDirty = true;

            Log::information(std::format("Preparing to unload chunk at ({}, {})", loadedChunk->chunkPosition.x, loadedChunk->chunkPosition.y));
        }

        // Load chunks
        if (!chunksToLoad.empty())
        {
            std::lock_guard lock(loadingThreadState.pendingTasksMutex);

            for (auto chunkToLoad : chunksToLoad)
            {
                ZoneScopedN("Chunk load");

                auto chunk = state.activeChunks.find(chunkToLoad);
                if (chunk != state.activeChunks.end())
                {
                    // Keep the chunk alive if necessary
                    chunk->second->isUnloading = false;

                    // Chunk is already loaded, we don't need to load
                    continue;
                }

                // Load the chunk
                state.activeChunks.emplace(chunkToLoad, std::make_unique<ActiveWorldChunk>(chunkToLoad, settings.chunkSize, state.scene));
                {
                    // Send a request to a worker thread to either load the chunk
                    loadingThreadState.pendingTasks.emplace(std::make_shared<ChunkLoadTask>(chunkToLoad, settings.chunkSize));
                    loadingThreadState.pendingTasksCondition.notify_one();
                }

                Log::verbose(std::format("Loading chunk at ({}, {})", chunkToLoad.x, chunkToLoad.y));
            }
        }

        state.isChunkLoadingDirty = false;
    }

    // Chunk unloading logic
    if (state.isChunkUnloadingDirty && settings.isChunkLoadingEnabled)
    {
        ZoneScopedN("Chunk unloading logic");

        // Check for chunks to unload
        int chunksUpdated = 0;
        std::vector<glm::ivec2> chunksToUnload {};
        for (auto& chunk : std::ranges::views::values(state.activeChunks))
        {
            if (chunk->isUnloading)
            {
                chunk->timeSpentWaitingForUnload += deltaTime;
                chunksUpdated++;

                if (chunk->timeSpentWaitingForUnload > settings.chunkUnloadTime)
                {
                    chunksToUnload.push_back(chunk->chunkPosition);
                }
            }
        }

        for (auto chunkToUnload : chunksToUnload)
        {
            ZoneScopedN("Chunk unload");

            Log::information(std::format("Unloaded chunk at ({}, {})", chunkToUnload.x, chunkToUnload.y));
            state.activeChunks.erase(chunkToUnload);
        }

        if (chunksUpdated == 0)
        {
            // No chunks have been updated this frame so we can stop checking until we are notified to start checking again
            state.isChunkUnloadingDirty = false;
        }
    }

    // Chunk data readback from worker threads
    {
        std::unique_lock completedRequestsLock(loadingThreadState.completedTasksMutex, std::defer_lock);
        if (completedRequestsLock.try_lock())
        {
            ZoneScopedN("Chunk load task readback");

            while (!loadingThreadState.completedTasks.empty())
            {
                ZoneScopedN("Chunk modification task creation");

                const auto task = loadingThreadState.completedTasks.front();
                loadingThreadState.completedTasks.pop();

                auto chunkIterator = state.activeChunks.find(task->chunkPosition);
                if (chunkIterator == state.activeChunks.end())
                {
                    // Chunk no longer exists (was probably unloaded)
                    // Throw the data away

                    continue;
                }

                auto& chunk = chunkIterator->second;
                chunk->isLoading = false;

                {
                    ZoneScopedN("Chunk modification task creation - Command buffer creation");

                    VoxelChunkCommandBuffer commandBuffer {};
                    commandBuffer.setSize(settings.chunkSize);
                    commandBuffer.copyFrom(task->chunkData);
                    commandBuffer.setExistsOnGpu(true);

                    submitCommandBuffer(chunk->component, commandBuffer);
                }
            }
        }
    }

    // Chunk visibility
    {
        // TODO: Don't require exclusive access
        std::lock_guard lock(state.scene->getMutex());
        auto camera = state.scene->camera;

        std::vector<std::shared_ptr<VoxelChunkComponent>> visibleChunks {};
        for (auto chunk : state.scene->allChunks)
        {
            auto isVisible = isOnScreen(chunk, camera);
            if (isVisible)
            {
                visibleChunks.push_back(chunk);
            }
        }

        state.scene->visibleChunks = visibleChunks;
        settings.isChunkLoadingEnabled = false;
    }
}

bool VoxelChunkManager::isOnScreen(const std::shared_ptr<VoxelChunkComponent>& chunk, const std::shared_ptr<CameraComponent>& camera)
{
    auto size = glm::vec3(chunk->getChunkData().getSize());
    if (size.x == 0 || size.y == 0 || size.z == 0)
    {
        return false;
    }

    std::vector cubeVertices {
        glm::vec3(-0.5f, -0.5f, -0.5f),
        glm::vec3(-0.5f, -0.5f, +0.5f),
        glm::vec3(-0.5f, +0.5f, -0.5f),
        glm::vec3(-0.5f, +0.5f, +0.5f),
        glm::vec3(+0.5f, -0.5f, -0.5f),
        glm::vec3(+0.5f, -0.5f, +0.5f),
        glm::vec3(+0.5f, +0.5f, -0.5f),
        glm::vec3(+0.5f, +0.5f, +0.5f),
    };

    // Transform into NDC space
    auto scale = glm::scale(glm::mat4(1), size);
    auto model = chunk->getTransform()->getGlobalTransform() * scale;
    auto view = camera->getTransform()->getInverseGlobalTransform();
    auto projection = glm::perspective(camera->getVerticalFov(), camera->getAspectRatio(), camera->getNearPlane(), camera->getFarPlane());
    auto modelView = view * model;
    auto modelViewProjection = projection * modelView;
    auto horizontalFovTan = glm::tan(camera->getHorizontalFov() / 2);
    auto verticalFovTan = glm::tan(camera->getVerticalFov() / 2);

    // For debugging
    for (int i = 0; i < cubeVertices.size(); ++i)
    {
        auto viewPosition = glm::vec3(modelView * glm::vec4(cubeVertices[i], 1));

        auto displayedVector = viewPosition;
        {
            ImGui::Text("%s", std::format("Vertex in view space: ({:.2f}, {:.2f}, {:.2f})", displayedVector.x, displayedVector.y, displayedVector.z).c_str());
        }
    }

    for (int i = 0; i < cubeVertices.size(); ++i)
    {
        auto viewPosition = glm::vec3(modelView * glm::vec4(cubeVertices[i], 1));
        viewPosition = glm::vec3(-viewPosition.y, viewPosition.z, -viewPosition.x);

        float x = (((viewPosition.x) / viewPosition.z)) / horizontalFovTan;
        float y = (((viewPosition.y) / viewPosition.z)) / horizontalFovTan / camera->getAspectRatio();
        // float x = glm::dot(glm::normalize(viewPosition) - glm::vec3(0, 0, -1), glm::vec3(1, 0, 0)) * horizontalFovTanInverse;
        // float y = glm::dot(glm::normalize(viewPosition) - glm::vec3(0, 0, -1), glm::vec3(0, 1, 0)) * horizontalFovTanInverse;
        float z = (viewPosition.z - camera->getNearPlane()) / camera->getFarPlane();

        cubeVertices[i] = glm::vec3(x, y, z);

        auto displayedVector = cubeVertices[i];
        if (displayedVector.z < 0)
        {
            ImDrawList* drawList = ImGui::GetBackgroundDrawList();

            auto windowSize = ImGuiUtility::toGlm(ImGui::GetIO().DisplaySize);
            auto pointPosition = glm::vec2((displayedVector.x + 1) / 2, 1 - ((displayedVector.y + 1) / 2)) * windowSize;
            auto color = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#ff0000")));

            drawList->AddCircleFilled(ImGuiUtility::toImGui(pointPosition), 5, color);
        }
    }

    // Case 1: Check for vertices
    for (int i = 0; i < cubeVertices.size(); ++i)
    {
        auto vertex = cubeVertices[i];
        auto isOnScreen = vertex.z < 0 && vertex.x > -1 && vertex.x < 1 && vertex.y > -1 && vertex.y < 1;

        ImGui::Text("%s", std::format("Vertex in screen space: ({:.2f}, {:.2f}, {:.2f})", vertex.x, vertex.y, vertex.z).c_str());
        ImGui::Text("%s", std::format("On screen?: {}", isOnScreen ? "Yes" : "No").c_str());

        // if (isOnScreen)
        // {
        //     return true;
        // }
    }

    return true;
}

void VoxelChunkManager::showDebugMenu()
{
    ZoneScoped;

    if (ImGui::CollapsingHeader("VoxelChunkManager"))
    {
        if (ImGui::Checkbox("Enable chunk loading", &settings.isChunkLoadingEnabled))
        {
            state.isChunkLoadingDirty = true;
        }

        if (ImGui::SliderInt("Render distance", &settings.renderDistance, 0, 1))
        {
            state.isChunkLoadingDirty = true;
        }

        ImGui::Text("%s", std::format("Render distance: {}", settings.renderDistance).c_str());
        ImGui::Text("%s", std::format("GPU uploaded voxel chunk count: {}", VoxelChunk::getInstanceCount()).c_str());
        ImGui::Text("%s", std::format("Loaded world chunk count: {}", state.activeChunks.size()).c_str());
        ImGui::Text("%s", std::format("Camera world chunk position: ({}, {})", state.cameraChunkPosition.x, state.cameraChunkPosition.y).c_str());
        ImGui::Text("%s", std::format("Chunk generation threads: {}", loadingThreadState.threads.size()).c_str());

        {
            // Chunk display distance parameters
            int displayDistance = settings.renderDistance + 1;

            glm::ivec2 displaySize = glm::ivec2(displayDistance * 2 + 1);
            glm::ivec2 displayCenter = glm::ivec2(displayDistance);

            // Drawing parameters
            float cellSize = 10;
            float dotSize = 6;
            float spacing = 2;

            auto* drawList = ImGui::GetWindowDrawList();
            auto windowPosition = ImGui::GetWindowPos();
            auto drawPosition = ImGui::GetCursorPos();

            auto unloadedCellColor = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#ff0000")));
            auto loadingCellColor = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#ffff00")));
            auto unloadingCellColor = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#333333")));
            auto loadedCellColor = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#00ff00")));
            auto loddedCellColor = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#0000ff")));

            auto defaultDotColor = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#00000000")));
            auto uploadedDotColor = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#000000")));
            auto visibleDotColor = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#ffffff")));

            auto baseDrawPosition = glm::vec2(windowPosition.x + drawPosition.x, windowPosition.y + drawPosition.y);

            for (int x = 0; x < displaySize.x; ++x)
            {
                for (int y = 0; y < displaySize.y; ++y)
                {
                    auto chunkPosition = state.cameraChunkPosition + glm::ivec2(x, y) - displayCenter;

                    // Cell
                    auto cellTopLeft = baseDrawPosition + (cellSize + spacing) * glm::vec2(x, y);
                    auto cellBottomRight = cellTopLeft + glm::vec2(cellSize);
                    auto cellColor = unloadedCellColor;

                    // Dot
                    auto dotPadding = (cellSize - dotSize) / 2;
                    auto dotTopLeft = cellTopLeft + glm::vec2(dotPadding);
                    auto dotBottomRight = cellBottomRight - glm::vec2(dotPadding);
                    auto dotColor = defaultDotColor;

                    auto chunkIterator = state.activeChunks.find(chunkPosition);
                    if (chunkIterator != state.activeChunks.end())
                    {
                        auto& chunk = chunkIterator->second;
                        cellColor = loadedCellColor;

                        if (chunk->isLoading)
                        {
                            cellColor = loadingCellColor;
                        }
                        else if (chunk->isUnloading)
                        {
                            cellColor = unloadingCellColor;
                        }

                        auto visibleChunks = state.scene->visibleChunks;
                        if (std::find(visibleChunks.begin(), visibleChunks.end(), chunk->component) != visibleChunks.end())
                        {
                            dotColor = visibleDotColor;
                        }
                        else if (chunk->component->getExistsOnGpu())
                        {
                            dotColor = uploadedDotColor;
                        }
                    }

                    drawList->AddRectFilled(ImGuiUtility::toImGui(cellTopLeft), ImGuiUtility::toImGui(cellBottomRight), cellColor);
                    drawList->AddRectFilled(ImGuiUtility::toImGui(dotTopLeft), ImGuiUtility::toImGui(dotBottomRight), dotColor);
                }
            }
        }
    }
}

void VoxelChunkManager::submitCommandBuffer(const std::shared_ptr<VoxelChunkComponent>& component, const VoxelChunkCommandBuffer& commandBuffer)
{
    std::lock_guard lock(modificationThreadState.pendingTasksMutex);

    modificationThreadState.pendingTasks.emplace(std::make_shared<ChunkModificationTask>(component, state.scene, commandBuffer));
    modificationThreadState.pendingTasksCondition.notify_one();
}
