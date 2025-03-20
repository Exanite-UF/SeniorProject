#include "VoxelChunkManager.h"

#include <algorithm>
#include <condition_variable>
#include <format>
#include <memory>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <src/utilities/Assert.h>
#include <src/utilities/ColorUtility.h>
#include <src/utilities/ImGui.h>
#include <src/utilities/ImGuiUtility.h>
#include <src/utilities/Log.h>
#include <src/world/VoxelChunkData.h>

VoxelChunkManager::~VoxelChunkManager()
{
    Log::log("Cleaning up VoxelChunkManager");

    isRunning = false;

    {
        std::lock_guard lock(data.pendingChunkLoadRequestsMutex);
        data.chunkLoadingThreadCondition.notify_all();
    }

    if (data.chunkLoadingThread.joinable())
    {
        data.chunkLoadingThread.join();
    }
}

void VoxelChunkManager::initialize(const std::shared_ptr<SceneComponent>& scene)
{
    Assert::isTrue(!isRunning, "VoxelChunkManager has already been initialized");

    isRunning = true;
    this->scene = scene;

    Log::log("Initializing VoxelChunkManager");

    data.chunkLoadingThread = std::thread(&VoxelChunkManager::chunkLoaderThreadEntrypoint, this);

    Log::log("Initialized VoxelChunkManager");
}

void VoxelChunkManager::chunkLoaderThreadEntrypoint()
{
    Log::log("Started VoxelChunkManager chunk loading thread");

    while (isRunning)
    {
        // Create pending requests lock, but don't lock the mutex
        // Locking is done by condition variable below
        std::unique_lock pendingRequestsLock(data.pendingChunkLoadRequestsMutex);

        // Wait for new requests to come in
        data.chunkLoadingThreadCondition.wait(pendingRequestsLock, [&]()
            {
                return !data.pendingRequests.empty() || !isRunning;
            });

        if (data.pendingRequests.empty() || !isRunning)
        {
            continue;
        }

        // Take some work
        auto request = data.pendingRequests.front();
        data.pendingRequests.pop();

        // Unlock the lock
        pendingRequestsLock.unlock();

        // Generate chunk
        Log::log(std::format("Generating chunk at ({}, {})", request->chunkPosition.x, request->chunkPosition.y));

        request->chunkData.setSize(request->chunkSize);
        // TODO: Actually generate chunk

        // Acquire completed requests mutex
        std::unique_lock completedRequestsLock(data.completedChunkLoadRequestsMutex);

        // Publish completed request
        data.completedRequests.push(request);
    }

    Log::log("Stopped VoxelChunkManager chunk loading thread");
}

void VoxelChunkManager::update(float deltaTime)
{
    // Calculate new camera chunk position
    data.cameraWorldPosition = scene->camera->getTransform()->getGlobalPosition();

    auto newCameraChunkPosition = glm::ivec2(glm::round(glm::vec2(
        data.cameraWorldPosition.x / Constants::VoxelChunkComponent::chunkSize.x - 0.5f,
        data.cameraWorldPosition.y / Constants::VoxelChunkComponent::chunkSize.y - 0.5f)));

    if (data.cameraChunkPosition != newCameraChunkPosition)
    {
        // Camera chunk position has changed, we may need to load new chunks
        data.isChunkLoadingDirty = true;
    }

    data.cameraChunkPosition = newCameraChunkPosition;

    if (data.isChunkLoadingDirty)
    {
        // Calculate which chunks should be loaded
        std::unordered_set<glm::ivec2> chunksToLoad {};
        for (int x = -data.renderDistance; x <= data.renderDistance; ++x)
        {
            for (int y = -data.renderDistance; y <= data.renderDistance; ++y)
            {
                auto chunkPosition = data.cameraChunkPosition + glm::ivec2(x, y);
                chunksToLoad.emplace(chunkPosition);
            }
        }

        // Unload chunks
        for (auto& loadedChunk : data.loadedChunks)
        {
            if (loadedChunk.second.isUnloading)
            {
                // Chunk is already unloading, skip it
                continue;
            }

            if (chunksToLoad.contains(loadedChunk.second.chunkPosition))
            {
                // Chunk should be loaded, don't unload it
                continue;
            }

            // Mark the chunk to be unloaded
            loadedChunk.second.isUnloading = true;
            data.isChunkUnloadingDirty = true;

            Log::log(std::format("Preparing to unload chunk at ({}, {})", loadedChunk.second.chunkPosition.x, loadedChunk.second.chunkPosition.y));
        }

        // Load chunks
        for (auto chunkToLoad : chunksToLoad)
        {
            auto loadedChunk = data.loadedChunks.find(chunkToLoad);
            if (loadedChunk != data.loadedChunks.end())
            {
                // Keep the chunk alive if necessary
                loadedChunk->second.isUnloading = false;

                // Chunk is already loaded, we don't need to load
                continue;
            }

            // Load the chunk
            data.loadedChunks.emplace(chunkToLoad, LoadedChunkData(chunkToLoad));
            // TODO: Acquire lock
            data.pendingRequests.emplace(std::make_shared<ChunkLoadRequest>(chunkToLoad, Constants::VoxelChunkComponent::chunkSize));
            // TODO: Send a job to a worker thread to either load the chunk from disk or generate the chunk

            Log::log(std::format("Loading chunk at ({}, {})", chunkToLoad.x, chunkToLoad.y));
        }
    }

    if (data.isChunkUnloadingDirty)
    {
        // Check for chunks to unload
        int chunksUpdated = 0;
        std::vector<glm::ivec2> chunksToUnload {};
        for (auto& loadedChunk : data.loadedChunks)
        {
            if (loadedChunk.second.isUnloading)
            {
                loadedChunk.second.unloadWaitTime += deltaTime;
                chunksUpdated++;

                if (loadedChunk.second.unloadWaitTime > data.chunkUnloadTime)
                {
                    chunksToUnload.push_back(loadedChunk.second.chunkPosition);
                }
            }
        }

        for (auto chunkToUnload : chunksToUnload)
        {
            Log::log(std::format("Unloaded chunk at ({}, {})", chunkToUnload.x, chunkToUnload.y));
            data.loadedChunks.erase(chunkToUnload);
            // TODO: Actually unload the chunk
        }

        if (chunksUpdated == 0)
        {
            // No chunks have been updated this frame so we can stop checking until we are notified to start checking again
            data.isChunkUnloadingDirty = false;
        }
    }
}

void VoxelChunkManager::showDebugMenu()
{
    if (ImGui::CollapsingHeader("VoxelChunkManager"))
    {
        ImGui::Text("%s", std::format("Render distance: {}", data.renderDistance).c_str());
        ImGui::Text("%s", std::format("Loaded chunk count: {}", data.loadedChunks.size()).c_str());
        ImGui::Text("%s", std::format("Camera chunk position: ({}, {})", data.cameraChunkPosition.x, data.cameraChunkPosition.y).c_str());

        {
            // Chunk display distance parameters
            int displayDistance = data.renderDistance * 2;

            glm::ivec2 displaySize = glm::ivec2(displayDistance * 2 + 1);
            glm::ivec2 displayCenter = glm::ivec2(displayDistance);

            // Drawing parameters
            float squareSize = 10;
            float padding = 2;

            auto* drawList = ImGui::GetWindowDrawList();
            auto windowPosition = ImGui::GetWindowPos();
            auto drawPosition = ImGui::GetCursorPos();

            auto unloadedColor = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#ff0000")));
            auto loadingColor = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#ffff00")));
            auto unloadingColor = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#333333")));
            auto loadedColor = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#00ff00")));
            auto loddedColor = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#0000ff")));

            auto baseDrawPosition = glm::vec2(windowPosition.x + drawPosition.x, windowPosition.y + drawPosition.y);

            for (int x = 0; x < displaySize.x; ++x)
            {
                for (int y = 0; y < displaySize.y; ++y)
                {
                    auto chunkPosition = data.cameraChunkPosition + glm::ivec2(x, y) - displayCenter;

                    auto topLeft = baseDrawPosition + (squareSize + padding) * glm::vec2(x, y);
                    auto bottomRight = topLeft + glm::vec2(squareSize);

                    auto color = unloadedColor;

                    auto chunkIterator = data.loadedChunks.find(chunkPosition);
                    if (chunkIterator != data.loadedChunks.end())
                    {
                        color = loadedColor;

                        if (chunkIterator->second.isUnloading)
                        {
                            color = unloadingColor;
                        }
                    }

                    drawList->AddRectFilled(ImGuiUtility::toImGui(topLeft), ImGuiUtility::toImGui(bottomRight), color);
                }
            }
        }
    }
}
