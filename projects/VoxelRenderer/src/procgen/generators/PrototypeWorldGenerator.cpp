#include <src/procgen/generators/PrototypeWorldGenerator.h>
#include <src/procgen/synthesizers/TextureOctaveNoiseSynthesizer.h>
#include <imgui/imgui.h>
#include <memory>
#include <src/procgen/PrintUtility.h>
#include <src/world/VoxelWorldData.h>

void PrototypeWorldGenerator::generateData()
{
    glm::ivec3 size = { data.getSize().x, data.getSize().y, 1 };
    this->textureData = std::make_shared<TextureData>(size);
    textureDataSynthesizer->generate(textureData);

    for (int x = 0; x < data.getSize().x; ++x)
    {
        for (int y = 0; y < data.getSize().y; ++y)
        {
            float noise = textureData->get(x, y);
            int offset = (int)(baseHeight + (noise * data.getSize().z));
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
    // TODO: Testing. Once finalized, add to existing Imgui fields.
    ImGui::PushID("PrototypeWorldGenerator");
    {
        if (ImGui::CollapsingHeader("Texture-Heightmap World Generator (F8)"))
        {
            ImGui::SliderFloat("Base Height", &baseHeight, 0, data.getSize().z);
            textureDataSynthesizer->showDebugMenu();
            if (ImGui::Button("Print Texture"))
            {
                PrintUtility::printTexture(textureData, textureDataSynthesizer->mapperTo01(), "output_texture.ppm");
            }
        }
    }
    ImGui::PopID();
}
