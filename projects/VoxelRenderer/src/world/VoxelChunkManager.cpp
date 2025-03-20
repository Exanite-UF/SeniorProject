#include "VoxelChunkManager.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <algorithm>
#include <format>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <src/utilities/Assert.h>
#include <src/utilities/ColorUtility.h>
#include <src/utilities/ImGui.h>
#include <src/utilities/ImGuiUtility.h>
#include <src/utilities/Log.h>

// TODO: Move these types into the header
struct LoadedChunk
{
public:
    std::shared_ptr<VoxelChunkComponent> chunk {};
};

struct Data
{
public:
    int generationDistance = 3; // TODO
    int renderDistance = 2;

    glm::vec3 cameraWorldPosition {};
    glm::ivec2 cameraChunkPosition {};

    bool isLoadingDirty = true; // If true, then we need to check for chunks to load/unload

    std::unordered_map<glm::ivec2, LoadedChunk> loadedChunks {};
};

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

void VoxelChunkManager::update()
{
    // Calculate new camera chunk position
    data.cameraWorldPosition = scene->camera->getTransform()->getGlobalPosition();

    auto newCameraChunkPosition = glm::ivec2(glm::round((glm::vec2(data.cameraWorldPosition) - glm::vec2(Constants::VoxelChunkComponent::chunkSize / 2)) / static_cast<float>(Constants::VoxelChunkComponent::chunkSize)));
    if (data.cameraChunkPosition != newCameraChunkPosition)
    {
        // Camera chunk position has changed, we may need to load new chunks
        data.isLoadingDirty = true;
    }

    data.cameraChunkPosition = newCameraChunkPosition;
}

void VoxelChunkManager::showDebugMenu()
{
    if (ImGui::CollapsingHeader("VoxelChunkManager"))
    {
        ImGui::Text("%s", std::format("Render distance: {}", data.renderDistance).c_str());
        ImGui::Text("%s", std::format("Loaded chunk count: {}", data.loadedChunks.size()).c_str());
        ImGui::Text("%s", std::format("Camera chunk position: ({}, {})", data.cameraChunkPosition.x, data.cameraChunkPosition.y).c_str());

        {
            int displayDistance = data.renderDistance * 2;
            int rowCount = 2 * displayDistance + 1;
            int columnCount = 2 * displayDistance + 1;
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

            auto basePosition = glm::vec2(windowPosition.x + drawPosition.x, windowPosition.y + drawPosition.y);

            for (int y = 0; y < rowCount; ++y)
            {
                for (int x = 0; x < columnCount; ++x)
                {
                    auto chunkPosition = data.cameraChunkPosition + glm::ivec2(x, y) - glm::ivec2(data.renderDistance + 1);

                    auto topLeft = basePosition + (squareSize + padding) * glm::vec2(x, y);
                    auto bottomRight = topLeft + glm::vec2(squareSize);

                    auto color = unloadedColor;

                    auto chunkIterator = data.loadedChunks.find(chunkPosition);
                    if (chunkIterator != data.loadedChunks.end())
                    {
                        color = loadedColor;
                    }

                    drawList->AddRectFilled(ImGuiUtility::toImGui(topLeft), ImGuiUtility::toImGui(bottomRight), color);
                }
            }
        }
    }
}
