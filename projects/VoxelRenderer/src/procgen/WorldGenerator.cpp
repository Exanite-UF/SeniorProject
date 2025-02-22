#include <src/procgen/WorldGenerator.h>
#include <imgui/imgui.h>
#include <src/PerlinNoise.hpp>
#include <src/world/VoxelWorldData.h>


WorldGenerator::WorldGenerator(glm::ivec3 worldSize)
{
    data.setSize(worldSize);
}

void WorldGenerator::generate(VoxelWorld& voxelWorld)
{
    //TODO: Remove copyFrom, and also set material. Otherwise voxels are black. 
    data.copyFrom(voxelWorld);

    data.clearOccupancy();

    siv::BasicPerlinNoise<float> perlinNoise(seed);

    for (int x = 0; x < data.getSize().x; ++x)
    {
        for (int y = 0; y < data.getSize().y; ++y)
        {
            float noise = perlinNoise.octave2D_01(((float)x) / data.getSize().x, ((float)y) / data.getSize().y, octaves, persistence);
            int offset = (int)(baseHeight + (noise * data.getSize().z));
            int height = glm::min(data.getSize().z, offset);

            for (int z = 0; z < height; ++z)
            {
                data.setVoxelOccupancy({ x, y, z }, true);
            }
        }
    }
    
    data.writeTo(voxelWorld);
}

void WorldGenerator::showDebugMenu()
{
    // TODO: Testing. Once finalized, add to existing Imgui fields.
    ImGui::SliderFloat("Seed", &seed, 0, 100);
    ImGui::SliderFloat("Base Height", &baseHeight, 0, data.getSize().z);
    ImGui::SliderInt("Octaves", &octaves, 1, 100);
    ImGui::SliderFloat("Persistence", &persistence, 0, 1);
}
