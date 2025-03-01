#include "ExaniteWorldGenerator.h"

#include <imgui/imgui.h>

ExaniteWorldGenerator::ExaniteWorldGenerator(glm::ivec3 worldSize)
    : WorldGenerator(worldSize)
{
}

void ExaniteWorldGenerator::generateData()
{
    for (int x = 0; x < data.getSize().x; ++x)
    {
        for (int y = 0; y < data.getSize().y; ++y)
        {
            data.setVoxelOccupancy(glm::ivec3(x, y, x), true);
            data.setVoxelMaterial(glm::ivec3(x, y, x), materialToUse);
        }
    }
}

void ExaniteWorldGenerator::showDebugMenu()
{
    if (ImGui::CollapsingHeader("Exanite's Generator"))
    {
        ImGui::SliderInt("Material", &materialToUse, 0, 1 << 9);
    }
}
