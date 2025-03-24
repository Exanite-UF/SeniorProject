#include <memory>

#include <PerlinNoise/PerlinNoise.hpp>

#include <FastNoiseLite/FastNoiseLite.h>
#include <src/procgen/PrintUtility.h>
#include <src/procgen/generators/PrototypeWorldGenerator.h>
#include <src/procgen/synthesizers/TextureOctaveNoiseSynthesizer.h>
#include <src/utilities/ImGui.h>
#include <src/utilities/Log.h>
#include <src/world/MaterialManager.h>
#include <src/world/VoxelChunkData.h>

void PrototypeWorldGenerator::generateData(VoxelChunkData& data)
{
    auto& materialManager = MaterialManager::getInstance();

    std::shared_ptr<Material> stoneMaterial;
    std::string stoneMaterialKey = "stone";
    if (!materialManager.tryGetMaterialByKey(stoneMaterialKey, stoneMaterial))
    {
        stoneMaterial = materialManager.getMaterialByIndex(0);
        Log::information("Failed to find stoneMaterial with id '" + stoneMaterialKey + "'. Using default stoneMaterial '" + stoneMaterial->getKey() + "' instead.");
    }

    std::shared_ptr<Material> lightStoneMaterial;
    std::string lightStoneMaterialKey = "lightStone";
    if (!materialManager.tryGetMaterialByKey(lightStoneMaterialKey, lightStoneMaterial))
    {
        stoneMaterial = materialManager.getMaterialByIndex(0);
        Log::information("Failed to find stoneMaterial with id '" + stoneMaterialKey + "'. Using default stoneMaterial '" + stoneMaterial->getKey() + "' instead.");
    }

    std::shared_ptr<Material> darkStoneMaterial;
    std::string darkStoneMaterialKey = "darkStone";
    if (!materialManager.tryGetMaterialByKey(darkStoneMaterialKey, darkStoneMaterial))
    {
        stoneMaterial = materialManager.getMaterialByIndex(0);
        Log::information("Failed to find stoneMaterial with id '" + darkStoneMaterialKey + "'. Using default stoneMaterial '" + stoneMaterial->getKey() + "' instead.");
    }

    std::shared_ptr<Material> dirtMaterial;
    std::string dirtMaterialKey = "dirt";
    if (!materialManager.tryGetMaterialByKey(dirtMaterialKey, dirtMaterial))
    {
        dirtMaterial = materialManager.getMaterialByIndex(0);
        Log::information("Failed to find Material with id '" + dirtMaterialKey + "'. Using default dirtMaterial '" + stoneMaterial->getKey() + "' instead.");
    }

    std::shared_ptr<Material> grassMaterial;
    std::string grassMaterialKey = "grass";
    if (!materialManager.tryGetMaterialByKey(grassMaterialKey, grassMaterial))
    {
        grassMaterial = materialManager.getMaterialByIndex(0);
        Log::information("Failed to find Material with id '" + grassMaterialKey + "'. Using default grassMaterial '" + stoneMaterial->getKey() + "' instead.");
    }

    // For organization
    auto placeBlock = [&data = data, &blockLength = blockLength](int x, int y, int z, std::function<std::shared_ptr<Material>(int, int, int)> blockFunction)
    {
        for (int localX = 0; localX < blockLength; ++localX)
        {
            for (int localY = 0; localY < blockLength; ++localY)
            {
                for (int localZ = 0; localZ < blockLength; ++localZ)
                {
                    glm::vec3 globalPos = { x + localX, y + localY, z + localZ };
                    data.setVoxelOccupancy(globalPos, true);
                    data.setVoxelMaterial(globalPos, blockFunction(localX, localY, localZ));
                }
            }
        }
    };

    // Fill texture data with random noise, each block evaluated once
    TextureData stoneBlockTextureData({ blockLength, blockLength, blockLength });
    FastNoiseLite simplexNoise;
    simplexNoise.SetSeed(seed);
    simplexNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    float stoneFrequency = 0.01;
    for (int localX = 0; localX < blockLength; localX++)
    {
        for (int localY = 0; localY < blockLength; localY++)
        {
            for (int localZ = 0; localZ < blockLength; localZ++)
            {
                // TODO: Analyze noise outputs
                float noise = simplexNoise.GetNoise(localX * stoneFrequency, localY * stoneFrequency, localZ * stoneFrequency);
                stoneBlockTextureData.set(noise, localX, localY, localZ);
            }
        }
    }

    // Capture all materials when evaluating a block? Maybe pass material manager reference on instance creation.
    auto stoneBlock = [stoneMaterial, lightStoneMaterial, darkStoneMaterial, &blockLength = blockLength, &stoneBlockTextureData](int localX, int localY, int localZ)
    {
        float sample = stoneBlockTextureData.get(localX, localY, localZ);
        if (sample > 0.7)
        {
            return lightStoneMaterial;
        }
        else if (sample > 0.2)
        {
            return stoneMaterial;
        }
        else
        {
            return darkStoneMaterial;
        }
    };
    auto dirtBlock = [grassMaterial, dirtMaterial, stoneMaterial, &blockLength = blockLength](int localX, int localY, int localZ)
    {
        return dirtMaterial;
    };
    auto grassBlock = [grassMaterial, dirtMaterial, stoneMaterial, &blockLength = blockLength](int localX, int localY, int localZ)
    {
        return grassMaterial;
    };

    siv::BasicPerlinNoise<float> perlinNoise(seed);

    // TODO: Fill entire space, don't floor
    int worldSizeBlocksX = std::floor(data.getSize().x / blockLength);
    int worldSizeBlocksY = std::floor(data.getSize().y / blockLength);
    int worldSizeBlocksZ = std::floor(data.getSize().z / blockLength);

    // Iterating by block since air has empty voxels that don't need to be filled anyways. Form of mipmapping?
    glm::vec2 offset = chunkSize * chunkPosition;
    for (int x = 0; x < worldSizeBlocksX; ++x)
    {
        for (int y = 0; y < worldSizeBlocksY; ++y)
        {
            // Stone Terrain, retain same shape as using voxels of blockLength 1
            float perlinNoiseSample = perlinNoise.octave2D_01((x * blockLength + offset.x) * frequency, (y * blockLength + offset.y) * frequency, octaves, persistence);
            int offsetBlocks = (int)(baseHeightBlocks + (perlinNoiseSample * terrainMaxAmplitudeBlocks));
            int heightBlocks = glm::min(data.getSize().z, offsetBlocks);

            for (int z = 0; z < heightBlocks; ++z)
            {
                placeBlock(x * blockLength, y * blockLength, z * blockLength, stoneBlock);
            }

            // Replace surface with grass
            int lastHeightBlocks = heightBlocks - 1;
            for (int z = lastHeightBlocks; z >= lastHeightBlocks - grassDepth && z >= 0; --z)
            {
                placeBlock(x * blockLength, y * blockLength, z * blockLength, grassBlock);
            }
            lastHeightBlocks -= grassDepth;

            // Replace surface with dirt
            for (int z = lastHeightBlocks; z >= lastHeightBlocks - dirtDepth && z >= 0; --z)
            {
                placeBlock(x * blockLength, y * blockLength, z * blockLength, dirtBlock);
            }
            lastHeightBlocks -= dirtDepth;
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
                ImGui::SliderInt("Base Height", &baseHeightBlocks, 0, chunkSize.z);
                ImGui::SliderInt("Octaves", &octaves, 1, 5);
                ImGui::SliderFloat("Persistence", &persistence, 0, 1);
                ImGui::SliderFloat("Frequency", &frequency, 0, 1);
                ImGui::SliderInt("Terrain Max Amplitude", &terrainMaxAmplitudeBlocks, 0, chunkSize.z);

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
