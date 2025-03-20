#include <src/procgen/generators/PrototypeWorldGenerator.h>
#include <imgui/imgui.h>
#include <memory>
#include <src/procgen/PrintUtility.h>
#include <src/world/VoxelWorldData.h>

#include <src/procgen/synthesizers/TextureOctaveNoiseSynthesizer.h>

void PrototypeWorldGenerator::generateData()
{
    glm::ivec3 size = { data.getSize().x, data.getSize().y, 1 };

    siv::BasicPerlinNoise<float> perlinNoise(seed);

    for (int x = 0; x < data.getSize().x; ++x)
    {
        for (int y = 0; y < data.getSize().y; ++y)
        {
            // Stone Terrain
            float perlinNoiseSample = perlinNoise.octave2D_01(x * frequency, y * frequency, octaves, persistence);
            int offset = (int)(baseHeight + (perlinNoiseSample * terrainMaxAmplitude));
            int height = glm::min(data.getSize().z, offset);

            for (int z = 0; z < height; ++z)
            {
                data.setVoxelOccupancy({ x, y, z }, true);
            }
        }
    }
}

void PrototypeWorldGenerator::showDebugMenu()
{
    // TODO: Testing. Once finalized, add to existing Imgwui fields.
    ImGui::PushID("PrototypeWorldGenerator");
    {
        if (ImGui::CollapsingHeader("Prototype World Generator (F8)"))
        {
            ImGui::SliderInt("Base Height", &baseHeight, 0, data.getSize().z);
            ImGui::SliderInt("Octaves", &octaves, 1, 5);
            ImGui::SliderFloat("Persistence", &persistence, 0, 1);
            ImGui::SliderFloat("Frequency", &frequency, 0, 1);
            
    int octaves = 3;
    float persistence = 0.5; 
    int baseHeight = 100;
    float frequency = 10;
    int terrainMaxAmplitude = 100;
            textureDataSynthesizer->showDebugMenu();
            if (ImGui::Button("Print Texture"))
            {
                PrintUtility::printTexture(textureData, textureDataSynthesizer->mapperTo01(), "output_texture.ppm");
            }
        }
    }
    ImGui::PopID();
}
