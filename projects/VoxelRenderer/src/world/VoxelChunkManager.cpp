#include "VoxelChunkManager.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <algorithm>
#include <format>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <src/utilities/Assert.h>
#include <src/utilities/ColorUtility.h>
#include <src/utilities/ImGui.h>
#include <src/utilities/ImGuiUtility.h>
#include <src/utilities/Log.h>

// TODO: Move these types into the header
struct LoadedChunkData
{
public:
    std::shared_ptr<VoxelChunkComponent> chunk {};
    glm::ivec2 chunkPosition;

    bool isUnloading = false;
    float unloadWaitTime = 0;

    explicit LoadedChunkData(const glm::ivec2& chunkPosition)
    {
        this->chunkPosition = chunkPosition;
    }
};

struct Data
{
public:
    // ----- Rendering -----

    // The distance at which chunks begin to be generated on a separate thread
    int generationDistance = 3; // TODO

    // The distance at which chunks are loaded and uploaded to the GPU
    int renderDistance = 2;

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

// TODO: Don't store this here
Data data;

VoxelChunkManager::~VoxelChunkManager()
{
    Log::log("Cleaning up VoxelChunkManager");
}

void VoxelChunkManager::initialize(const std::shared_ptr<SceneComponent>& scene)
{
    Assert::isTrue(!isInitialized, "VoxelChunkManager has already been initialized");

    isInitialized = true;

    this->scene = scene;

    Log::log("Initialized VoxelChunkManager");
}

void VoxelChunkManager::update(float deltaTime)
{
    // Calculate new camera chunk position
    data.cameraWorldPosition = scene->camera->getTransform()->getGlobalPosition();

    auto newCameraChunkPosition = glm::ivec2(glm::round((glm::vec2(data.cameraWorldPosition) - glm::vec2(Constants::VoxelChunkComponent::chunkSize / 2)) / static_cast<float>(Constants::VoxelChunkComponent::chunkSize)));
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
            if (chunksToLoad.contains(loadedChunk.second.chunkPosition))
            {
                // Chunk should be loaded, don't unload it
                continue;
            }

            // Mark the chunk to be unloaded
            loadedChunk.second.isUnloading = true;
            data.isChunkLoadingDirty = true;

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
