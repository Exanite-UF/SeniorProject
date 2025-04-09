#include <cmath>
#include <format>
#include <imgui/imgui.h>
#include <src/procgen/data/FlatArrayData.h>
#include <src/procgen/synthesizers/GridPointSynthesizer.h>
#include <src/utilities/Log.h>
#include <typeinfo>

void GridPointSynthesizer::generatePoints(std::vector<glm::vec3>& inPoints, uint32_t numPoints)
{
    int squareGridLength = (int)std::ceil(std::sqrt(numPoints));

    for (int x = 0; x < squareGridLength; x++)
    {
        for (int y = 0; y < squareGridLength; y++)
        {
            glm::vec3 point = { (float)x / squareGridLength, (float)y / squareGridLength, 0 };
            inPoints.push_back(point);
            Log::verbose(std::format("Grid Point [0-1]: ({:.2f}, {:.2f})", point.x, point.y));
        }
    }
}

void GridPointSynthesizer::rescalePointsToChunkSize(std::vector<glm::vec3>& inPoints, VoxelChunkData& chunkData)
{
    for (int i = 0; i < inPoints.size(); i++)
    {
        glm::vec3& point = inPoints[i];
        inPoints[i] = glm::vec3({ std::ceil((point.x / rangeX.y) * chunkData.getSize().x), std::ceil((point.y / rangeY.y) * chunkData.getSize().y), 0 });
        Log::verbose(std::format("Grid Point Rescaled: ({:.2f}, {:.2f})", point.x, point.y));
    }
}

void GridPointSynthesizer::showDebugMenu()
{
    ImGui::PushID(typeid(GridPointSynthesizer).name());
    {
        if (ImGui::CollapsingHeader(typeid(GridPointSynthesizer).name()))
        {
            ImGui::SliderInt("Seed", &seed, 0, 100);
        }
    }
    ImGui::PopID();
}
