#include <src/procgen/synthesizers/PoissonDiskPointSynthesizer.h>

#include <PoissonDiskGenerator/PoissonGenerator.h>
#include <algorithm>
#include <format>
#include <glm/vec3.hpp>
#include <imgui/imgui.h>
#include <iostream>
#include <src/utilities/Log.h>
#include <typeinfo>

void PoissonDiskPointSynthesizer::generatePoints(std::vector<glm::vec3>& outPoints, uint32_t numPoints)
{
    PoissonGenerator::DefaultPRNG PRNG(seed);

    const auto points = PoissonGenerator::generatePoissonPoints(numPoints, PRNG, false);

    for (int i = 0; i < points.size(); i++)
    {
        outPoints.push_back(glm::vec3(points[i].x, points[i].y, 0));
        Log::verbose(std::format("Poisson [0-1]: ({:.2f}, {:.2f})", outPoints[i].x, outPoints[i].y));
    }
}

void PoissonDiskPointSynthesizer::rescalePointsToChunkSize(std::vector<glm::vec3>& outPoints, VoxelChunkData& chunkData)
{
    for (int i = 0; i < outPoints.size(); i++)
    {
        glm::vec3& point = outPoints[i];
        outPoints[i] = glm::vec3({ std::ceil((point.x / rangeX.y) * chunkData.getSize().x), std::ceil((point.y / rangeY.y) * chunkData.getSize().y), 0 });
        Log::verbose(std::format("Poisson Rescaled: {:.2f}, {:.2f}", outPoints[i].x, outPoints[i].y));
    }
}

void PoissonDiskPointSynthesizer::showDebugMenu()
{
    ImGui::PushID(typeid(PoissonDiskPointSynthesizer).name());
    {
        if (ImGui::CollapsingHeader(typeid(PoissonDiskPointSynthesizer).name()))
        {
            ImGui::SliderInt("Seed", &seed, 0, 100);
        }
    }
    ImGui::PopID();
}
