#include <PerlinNoise/PerlinNoise.hpp>
#include <imgui/imgui.h>
#include <src/procgen/OctaveNoiseWorldGenerator.h>
#include <src/world/VoxelWorldData.h>

void OctaveNoiseWorldGenerator::generateData()
{
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
}

void OctaveNoiseWorldGenerator::showDebugMenu()
{
    // TODO: Testing. Once finalized, add to existing Imgui fields.
    if (ImGui::CollapsingHeader("Octave Noise Generator"))
    {
        ImGui::SliderFloat("Seed", &seed, 0, 100);
        ImGui::SliderFloat("Base Height", &baseHeight, 0, data.getSize().z);
        ImGui::SliderInt("Octaves", &octaves, 1, 100);
        ImGui::SliderFloat("Persistence", &persistence, 0, 1);
    }
}
