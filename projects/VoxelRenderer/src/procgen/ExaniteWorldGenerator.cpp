#include "ExaniteWorldGenerator.h"

#include <imgui/imgui.h>
#include <src/utilities/Assert.h>
#include <src/utilities/MeasureElapsedTimeScope.h>
#include <src/world/MaterialManager.h>

ExaniteWorldGenerator::ExaniteWorldGenerator(glm::ivec3 worldSize)
    : WorldGenerator(worldSize)
{
}

void ExaniteWorldGenerator::generateData()
{
    MeasureElapsedTimeScope scope("ExaniteWorldGenerator::generateData");

    // Iterate through each 16x16x16 region
    auto chunkSize = data.getSize();
    auto palette2RegionCount = chunkSize >> 4;
    for (int palette2ZI = 0; palette2ZI < palette2RegionCount.z; ++palette2ZI)
    {
        for (int palette2YI = 0; palette2YI < palette2RegionCount.y; ++palette2YI)
        {
            for (int palette2XI = 0; palette2XI < palette2RegionCount.x; ++palette2XI)
            {
                auto materialOffset2 = (((palette2ZI >> 1) & 1) * 256) + (((palette2ZI & 1) << 2) | ((palette2YI & 1) << 1) | ((palette2XI & 1) << 0));

                // The code inside this block represents a 16x16x16 region (256 materials will be used per region)
                // Set the materials in each 16x16x4 layer in the following pattern (64 materials will be used per layer):
                // 00114455
                // 00114455
                // 22336677
                // 22336677
                // ...4 more rows
                for (int palette1ZI = 0; palette1ZI < 4; ++palette1ZI)
                {
                    for (int palette1YI = 0; palette1YI < 4; ++palette1YI)
                    {
                        for (int palette1XI = 0; palette1XI < 4; ++palette1XI)
                        {
                            int xyRegion1 = (((palette1YI >> 1) & 1) << 1) | (((palette1XI >> 1) & 1) << 0);
                            int materialOffset1 = 64 * palette1ZI + 16 * xyRegion1;

                            // The code inside this block represents a 4x4x4 region (16 materials will be used per region)
                            // Set the materials in each 4x4x1 layer in the following pattern (4 materials will be used per layer):
                            // 0011
                            // 0011
                            // 2233
                            // 2233
                            for (int palette0ZI = 0; palette0ZI < 4; ++palette0ZI)
                            {
                                for (int palette0YI = 0; palette0YI < 4; ++palette0YI)
                                {
                                    for (int palette0XI = 0; palette0XI < 4; ++palette0XI)
                                    {
                                        int xyRegion0 = (((palette0YI >> 1) & 1) << 1) | (((palette0XI >> 1) & 1) << 0);
                                        int materialOffset0 = 4 * palette0ZI + 1 * xyRegion0;

                                        // The code inside this block represents a single voxel
                                        int x = palette2XI * 16 + palette1XI * 4 + palette0XI;
                                        int y = palette2YI * 16 + palette1YI * 4 + palette0YI;
                                        int z = palette2ZI * 16 + palette1ZI * 4 + palette0ZI;

                                        int materialIndex = materialOffset2 + materialOffset1 + materialOffset0;
                                        if (materialIndex >= 16)
                                        {
                                            continue;
                                        }

                                        data.setVoxelMaterial(glm::ivec3(x, y, z), materialIndex % 512);
                                    }
                                }
                            }
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
    ImGui::PushID("ExaniteWorldGenerator");
    {
        if (ImGui::CollapsingHeader("Exanite's Generator (F6)"))
        {
            ImGui::Text("This generator is used to test material palette solving");
        }
    }
    ImGui::PopID();
}
