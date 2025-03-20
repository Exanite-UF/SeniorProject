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

struct CameraData
{
public:
    glm::ivec3 previousPosition {};
};

struct LoadedChunk
{
public:
    std::shared_ptr<VoxelChunkComponent> chunk {};
};

struct Data
{
public:
    int renderDistance = 2;
    CameraData cameraData {};
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
}

void VoxelChunkManager::showDebugMenu()
{
    if (ImGui::CollapsingHeader("VoxelChunkManager"))
    {
        ImGui::Text("%s", std::format("Render distance: {}", data.renderDistance).c_str());
        ImGui::Text("%s", std::format("Loaded chunk count: {}", data.loadedChunks.size()).c_str());

        {
            int rowCount = 2 * data.renderDistance + 1;
            int columnCount = 2 * data.renderDistance + 1;
            float squareSize = 10;
            float padding = 2;

            auto* drawList = ImGui::GetWindowDrawList();
            auto windowPosition = ImGui::GetWindowPos();
            auto drawPosition = ImGui::GetCursorPos();

            auto unloadedColor = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#ff0000")));
            auto loddedColor = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#ffff00")));
            auto loadedColor = ImGui::ColorConvertFloat4ToU32(ImGuiUtility::toImGui(ColorUtility::htmlToSrgb("#00ff00")));

            auto basePosition = glm::vec2(windowPosition.x + drawPosition.x, windowPosition.y + drawPosition.y);

            for (int y = 0; y < rowCount; ++y)
            {
                for (int x = 0; x < columnCount; ++x)
                {
                    auto topLeft = basePosition + (squareSize + padding) * glm::vec2(x, y);
                    auto bottomRight = topLeft + glm::vec2(squareSize);

                    drawList->AddRectFilled(ImGuiUtility::toImGui(topLeft), ImGuiUtility::toImGui(bottomRight), unloadedColor);
                }
            }
        }
    }
}
