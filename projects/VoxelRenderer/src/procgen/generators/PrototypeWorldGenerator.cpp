#include <memory>

#include <PerlinNoise/PerlinNoise.hpp>

#include <src/procgen/PrintUtility.h>
#include <src/procgen/generators/PrototypeWorldGenerator.h>
#include <src/procgen/synthesizers/TextureOctaveNoiseSynthesizer.h>
#include <src/utilities/ImGui.h>
#include <src/utilities/Log.h>
#include <src/world/MaterialManager.h>
#include <src/world/VoxelChunkData.h>

void PrototypeWorldGenerator::generateData(VoxelChunkData& data)
{
    glm::ivec3 size = { data.getSize().x, data.getSize().y, 1 };

    auto& materialManager = MaterialManager::getInstance();

    std::shared_ptr<Material> stoneMaterial;
    std::string stoneMaterialKey = "stone";
    if (!materialManager.tryGetMaterialByKey(stoneMaterialKey, stoneMaterial))
    {
        stoneMaterial = materialManager.getMaterialByIndex(0);
        Log::log("Failed to find stoneMaterial with id '" + stoneMaterialKey + "'. Using default stoneMaterial '" + stoneMaterial->getKey() + "' instead.");
    }

    std::shared_ptr<Material> dirtMaterial;
    std::string dirtMaterialKey = "dirt";
    if (!materialManager.tryGetMaterialByKey(dirtMaterialKey, dirtMaterial))
    {
        dirtMaterial = materialManager.getMaterialByIndex(0);
        Log::log("Failed to find Material with id '" + dirtMaterialKey + "'. Using default dirtMaterial '" + stoneMaterial->getKey() + "' instead.");
    }

    std::shared_ptr<Material> grassMaterial;
    std::string grassMaterialKey = "grass";
    if (!materialManager.tryGetMaterialByKey(grassMaterialKey, grassMaterial))
    {
        grassMaterial = materialManager.getMaterialByIndex(0);
        Log::log("Failed to find Material with id '" + grassMaterialKey + "'. Using default grassMaterial '" + stoneMaterial->getKey() + "' instead.");
    }

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
                data.setVoxelMaterial({ x, y, z }, stoneMaterial);
            }

            // Replace surface with grass
            int lastHeight = height - 1;
            for (int z = lastHeight; z >= lastHeight - grassDepth && z >= 0; --z)
            {
                data.setVoxelMaterial({ x, y, z }, grassMaterial);
            }
            lastHeight -= grassDepth;

            // Replace surface with dirt
            for (int z = lastHeight; z >= lastHeight - dirtDepth && z >= 0; --z)
            {
                data.setVoxelMaterial({ x, y, z }, dirtMaterial);
            }
            lastHeight -= dirtDepth;
        }
    }
}

PrototypeWorldGenerator::PrototypeWorldGenerator(const std::shared_ptr<TextureDataSynthesizer>& textureDataSynthesizer)
{
    this->textureDataSynthesizer = textureDataSynthesizer;
}

void PrototypeWorldGenerator::showDebugMenu()
{
    // TODO: Testing. Once finalized, add to existing Imgui fields.
    ImGui::PushID("PrototypeWorldGenerator");
    {
        if (ImGui::CollapsingHeader("Prototype World Generator (F8)"))
        {
            if (ImGui::BeginMenu("Stone Terrain"))
            {
                ImGui::SliderInt("Base Height", &baseHeight, 0, chunkSize.z);
                ImGui::SliderInt("Octaves", &octaves, 1, 5);
                ImGui::SliderFloat("Persistence", &persistence, 0, 1);
                ImGui::SliderFloat("Frequency", &frequency, 0, 1);
                ImGui::SliderInt("Terrain Max Amplitude", &terrainMaxAmplitude, 0, chunkSize.z);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Surface Dirt"))
            {
                ImGui::SliderInt("Grass Depth", &grassDepth, 0, 20);
                ImGui::SliderInt("Dirt Depth", &dirtDepth, 0, 20);

                ImGui::EndMenu();
            }

            textureDataSynthesizer->showDebugMenu();
            if (ImGui::Button("Print Texture"))
            {
                PrintUtility::printTexture(textureData, textureDataSynthesizer->mapperTo01(), "output_texture.ppm");
            }
        }
    }
    ImGui::PopID();
}
