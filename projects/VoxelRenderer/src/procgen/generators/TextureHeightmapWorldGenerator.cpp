#include <src/procgen/generators/TextureHeightmapWorldGenerator.h>
#include <PerlinNoise/PerlinNoise.hpp>
#include <imgui/imgui.h>
#include <src/world/VoxelWorldData.h>

void TextureHeightmapWorldGenerator::generateData()
{
    TextureData textureData({ data.getSize().x, data.getSize().y, 1 });
    textureDataSynthesizer->generate(textureData);

    for (int x = 0; x < data.getSize().x; ++x)
    {
        for (int y = 0; y < data.getSize().y; ++y)
        {
            float noise = textureData.get(x, y);
            int offset = (int)(baseHeight + (noise * data.getSize().z));
            int height = glm::min(data.getSize().z, offset);

            for (int z = 0; z < height; ++z)
            {
                data.setVoxelOccupancy({ x, y, z }, true);
            }
        }
    }
}

void TextureHeightmapWorldGenerator::showDebugMenu()
{
    // TODO: Testing. Once finalized, add to existing Imgui fields.
    ImGui::PushID("TextureHeightmapWorldGenerator");
    {
        if (ImGui::CollapsingHeader("Texture-Heightmap World Generator (F8)"))
        {
            ImGui::SliderFloat("Base Height", &baseHeight, 0, data.getSize().z);
            textureDataSynthesizer->showDebugMenu();
        }
    }
    ImGui::PopID();
}
