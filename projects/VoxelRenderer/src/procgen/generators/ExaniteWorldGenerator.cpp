#include "ExaniteWorldGenerator.h"

#include <imgui/imgui.h>
#include <src/utilities/Assert.h>
#include <src/utilities/MeasureElapsedTimeScope.h>
#include <src/world/MaterialManager.h>

void ExaniteWorldGenerator::generateData(VoxelChunkData& data)
{
    MeasureElapsedTimeScope scope("ExaniteWorldGenerator::generateData");

    // Iterate through each 16x16x16 region
    auto chunkCount = data.getSize() >> 4;
    for (int chunkZI = 0; chunkZI < chunkCount.z; ++chunkZI)
    {
        for (int chunkYI = 0; chunkYI < chunkCount.y; ++chunkYI)
        {
            for (int chunkXI = 0; chunkXI < chunkCount.x; ++chunkXI)
            {
                // The code inside this block represents a 16x16x16 region
                for (int zI = 0; zI < 16; ++zI)
                {
                    for (int yI = 0; yI < 16; ++yI)
                    {
                        for (int xI = 0; xI < 16; ++xI)
                        {
                            // The code inside this block represents a single voxel
                            glm::ivec3 size2 = data.getSize() >> 4;

                            glm::ivec3 position0 = glm::ivec3(chunkXI * 16 + xI, chunkYI * 16 + yI, chunkZI * 16 + zI);
                            glm::ivec3 position1 = position0 >> 2;
                            glm::ivec3 position2 = position0 >> 4;

                            int materialBits0 = ((position0.z & 1) << 2) | ((position0.y & 1) << 1) | ((position0.x & 1) << 0);
                            int materialBits1 = ((position1.z & 1) << 2) | ((position1.y & 1) << 1) | ((position1.x & 1) << 0);
                            int materialBits2 = ((position2.z & 1) << 2) | ((position2.y & 1) << 1) | ((position2.x & 1) << 0);

                            int materialOffset = position2.x + size2.x * (position2.y + size2.y * position2.z);

                            int materialIndex = ((materialBits2 << 6) | (materialBits1 << 3) | (materialBits0 << 0)) + materialOffset;

                            data.setVoxelMaterialIndex(position0, materialIndex % Constants::VoxelChunk::maxMaterialCount);
                        }
                    }
                }
            }
        }
    }

    for (int z = 0; z < data.getSize().z; ++z)
    {
        for (int y = 0; y < data.getSize().y; ++y)
        {
            for (int x = 0; x < data.getSize().x; ++x)
            {
                data.setVoxelOccupancy(glm::ivec3(x, y, z), true);
            }
        }
    }
}

void ExaniteWorldGenerator::showDebugMenu()
{
    // To Fix the Long title issue for headers
    std::string headerText = "Exanite's Generator";

    float availableWidth = ImGui::GetContentRegionAvail().x;
    float textWidth = ImGui::CalcTextSize(headerText.c_str()).x;

    if (textWidth > availableWidth)
    {
        while (ImGui::CalcTextSize((headerText + "...").c_str()).x > availableWidth && headerText.length() > 3)
        {
            headerText.pop_back();
        }
        headerText += "...";
    }


    ImGui::PushID("ExaniteWorldGenerator");
    {
        ImGui::PushTextWrapPos(ImGui::GetWindowContentRegionMax().x);
        if (ImGui::CollapsingHeader(headerText.c_str()))
        {
            ImGui::Text("This generator is used to test material palette solving");
        }
        ImGui::PopTextWrapPos();
    }
    ImGui::PopID();
}
