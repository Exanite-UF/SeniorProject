#include <memory>

#include <PerlinNoise/PerlinNoise.hpp>

#include <FastNoiseLite/FastNoiseLite.h>
#include <cstdlib>
#include <src/procgen/PrintUtility.h>
#include <src/procgen/data/FlatArrayData.h>
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

    std::shared_ptr<Material> dirtMaterial;
    std::string dirtMaterialKey = "dirt";
    if (!materialManager.tryGetMaterialByKey(dirtMaterialKey, dirtMaterial))
    {
        dirtMaterial = materialManager.getMaterialByIndex(0);
        Log::information("Failed to find Material with id '" + dirtMaterialKey + "'. Using default dirtMaterial '" + dirtMaterial->getKey() + "' instead.");
    }

    std::shared_ptr<Material> grassMaterial;
    std::string grassMaterialKey = "grass";
    if (!materialManager.tryGetMaterialByKey(grassMaterialKey, grassMaterial))
    {
        grassMaterial = materialManager.getMaterialByIndex(0);
        Log::information("Failed to find Material with id '" + grassMaterialKey + "'. Using default grassMaterial '" + grassMaterial->getKey() + "' instead.");
    }

    std::shared_ptr<Material> oakLogMaterial;
    std::string oakLogMaterialKey = "oak_log";
    if (!materialManager.tryGetMaterialByKey(oakLogMaterialKey, oakLogMaterial))
    {
        oakLogMaterial = materialManager.getMaterialByIndex(0);
        Log::information("Failed to find Material with id '" + oakLogMaterialKey + "'. Using default oakLogMaterial '" + oakLogMaterial->getKey() + "' instead.");
    }

    std::shared_ptr<Material> oakLeafMaterial;
    std::string oakLeafMaterialKey = "oak_leaf";
    if (!materialManager.tryGetMaterialByKey(oakLeafMaterialKey, oakLeafMaterial))
    {
        oakLeafMaterial = materialManager.getMaterialByIndex(0);
        Log::information("Failed to find Material with id '" + oakLeafMaterialKey + "'. Using default oakLeafMaterial '" + oakLeafMaterial->getKey() + "' instead.");
    }

    // Fill texture data with random noise, each block evaluated once
    FastNoiseLite simplexNoise;
    simplexNoise.SetSeed(seed);
    simplexNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    float stoneFrequency = 0.01;
    // float noise = simplexNoise.GetNoise(localX * stoneFrequency, localY * stoneFrequency, localZ * stoneFrequency);

    siv::BasicPerlinNoise<float> perlinNoise(seed);

    glm::vec3 treeLocation({ data.getSize().x / 2, data.getSize().y / 2, 0 });
    int voxelsPerMeter = 8;

    int minTreeHeightVoxels = 5 * voxelsPerMeter;
    int maxTreeHeightVoxels = 7 * voxelsPerMeter;

    int minTreeWidthVoxels = 1 * voxelsPerMeter;
    int maxTreeWidthVoxels = 2 * voxelsPerMeter;

    int minLeafWidthX = 4 * voxelsPerMeter;
    int maxLeafWidthX = 6 * voxelsPerMeter;
    
    int minLeafWidthY = 4 * voxelsPerMeter;
    int maxLeafWidthY = 6 * voxelsPerMeter;

    int minLeafExtentBelowZ = 0 * voxelsPerMeter;
    int maxLeafExtentBelowZ = 0 * voxelsPerMeter;

    int minLeafExtentAboveZ = 1 * voxelsPerMeter;
    int maxLeafExtentAboveZ = 1 * voxelsPerMeter;

    // Iterating by block since air has empty voxels that don't need to be filled anyways. Form of mipmapping?
    glm::vec2 offset = chunkSize * chunkPosition;
    for (int x = 0; x < data.getSize().x; ++x)
    {
        for (int y = 0; y < data.getSize().y; ++y)
        {
            // Stone Terrain, retain same shape as using voxels of blockLength 1
            float perlinNoiseSample = perlinNoise.octave2D_01((x + offset.x) * frequency, (y + offset.y) * frequency, octaves, persistence);
            int offsetVoxels = (int)(baseHeightBlocks + (perlinNoiseSample * terrainMaxAmplitudeBlocks));
            int heightVoxels = glm::min(data.getSize().z, offsetVoxels);

            for (int z = 0; z < heightVoxels; ++z)
            {
                data.setVoxelOccupancy({ x, y, z }, true);
                data.setVoxelMaterial({ x, y, z }, stoneMaterial);
            }

            // Replace surface with grass
            int lastHeightBlocks = heightVoxels - 1;
            for (int z = lastHeightBlocks; z >= lastHeightBlocks - grassDepth && z >= 0; --z)
            {
                data.setVoxelOccupancy({ x, y, z }, true);
                data.setVoxelMaterial({ x, y, z }, grassMaterial);
            }
            lastHeightBlocks -= grassDepth;

            // Replace surface with dirt
            for (int z = lastHeightBlocks; z >= lastHeightBlocks - dirtDepth && z >= 0; --z)
            {
                data.setVoxelOccupancy({ x, y, z }, true);
                data.setVoxelMaterial({ x, y, z }, dirtMaterial);
            }
            lastHeightBlocks -= dirtDepth;

            if (treeLocation.x == x && treeLocation.y == y)
            { 
                glm::vec3 originVoxel({ x, y, heightVoxels });

                // Naive seeding. Is there a better way?
                std::srand(seed + chunkPosition.x + chunkPosition.y * 10 + chunkPosition.z * 100 + originVoxel.x * 11 + originVoxel.y * 11);

                int treeHeightVoxels = randomBetween(minTreeHeightVoxels, maxTreeHeightVoxels); 
                int treeWidthVoxels = randomBetween(minTreeWidthVoxels, maxTreeWidthVoxels);
                int leafWidthX = randomBetween(minLeafWidthX, maxLeafWidthX);
                int leafWidthY = randomBetween(minLeafWidthY, maxLeafWidthY);
                int leafExtentBelowZ = randomBetween(minLeafExtentBelowZ, maxLeafExtentBelowZ);
                int leafExtentAboveZ = randomBetween(minLeafExtentAboveZ, maxLeafExtentAboveZ);

                generateTree(data, oakLogMaterial, oakLeafMaterial, originVoxel, treeHeightVoxels, treeWidthVoxels, leafWidthX, leafWidthY, leafExtentBelowZ, leafExtentAboveZ);
            }
        }
    }
}


int PrototypeWorldGenerator::randomBetween(int min, int max)
{
    return min + rand() % (max - min + 1);
}

void PrototypeWorldGenerator::generateTree(VoxelChunkData& data, std::shared_ptr<Material>& logMaterial, std::shared_ptr<Material>& leafMaterial, glm::vec3 originVoxel, int treeHeightVoxels, int treeWidthVoxels, int leafWidthX, int leafWidthY, int leafWidthExtentBelowZ, int leafWidthExtentAboveZ)
{
    // Tree Trunk
    int treeWidthRadius = treeWidthVoxels / 2;

    for (int localX = -treeWidthRadius; localX <= treeWidthRadius; ++localX)
    {
        for (int localY = -treeWidthRadius; localY <= treeWidthRadius; ++localY)
        {
            for (int localZ = 0; localZ <= treeHeightVoxels; ++localZ)
            {
                glm::vec3 localVoxel = { originVoxel.y + localX, originVoxel.y + localY, originVoxel.z + localZ };

                // Fall through
                if (localVoxel.x <= 0 || localVoxel.y <= 0 || localVoxel.z <= 0)
                {
                    continue;
                }

                if (localVoxel.x > data.getSize().x || localVoxel.y > data.getSize().y || localVoxel.z > data.getSize().z)
                {
                    continue;
                }

                data.setVoxelOccupancy(localVoxel, true);
                data.setVoxelMaterial(localVoxel, logMaterial);
            }
        }
    }

    glm::vec3 originOffset = { 0, 0, treeHeightVoxels + 1 };
    originVoxel += originOffset;

    int leafWidthRadiusX = leafWidthX / 2;
    int leafWidthRadiusY = leafWidthY / 2;

    // Setup tree function
    int height = leafWidthExtentAboveZ;
    int heightToWidthXRatio = (height) / leafWidthX;
    int heightToWidthYRatio = (height) / leafWidthY;

    for (int localX = -leafWidthRadiusX; localX <= leafWidthRadiusX; ++localX)
    {
        for (int localY = -leafWidthRadiusY; localY <= leafWidthRadiusY; ++localY)
        {
            for (int localZ = -leafWidthExtentBelowZ; localZ <= leafWidthExtentAboveZ; ++localZ)
            {
                glm::vec3 localVoxel = { originVoxel.y + localX, originVoxel.y + localY, originVoxel.z + localZ };

                // Fall through
                if (localVoxel.x <= 0 || localVoxel.y <= 0 || localVoxel.z <= 0)
                {
                    continue;
                }

                if (localVoxel.x > data.getSize().x || localVoxel.y > data.getSize().y || localVoxel.z > data.getSize().z)
                {
                    continue;
                }

                // Sample from tree function
                float treeFunctionSample = height - heightToWidthXRatio * abs(localX) - heightToWidthYRatio * abs(localY) - localZ;
                // Simple random function. Probably better to clump and also add so it looks more organic.
                int randomSample = (rand() % 10);

                if (treeFunctionSample > 0 && randomSample > 6)
                {
                    if (data.getVoxelMaterial(localVoxel) != logMaterial)
                    {
                        data.setVoxelOccupancy(localVoxel, true);
                        data.setVoxelMaterial(localVoxel, leafMaterial);
                    }
                }
            }
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
