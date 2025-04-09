#include <memory>

#include <PerlinNoise/PerlinNoise.hpp>

#include <FastNoiseLite/FastNoiseLite.h>
#include <cstdlib>
#include <src/procgen/PrintUtility.h>
#include <src/procgen/WorldUtility.h>
#include <src/procgen/data/FlatArrayData.h>
#include <src/procgen/data/TreeStructure.h>
#include <src/procgen/generators/PrototypeWorldGenerator.h>
#include <src/procgen/synthesizers/PoissonDiskPointSynthesizer.h>
#include <src/procgen/synthesizers/GridPointSynthesizer.h>
#include <src/procgen/synthesizers/TextureOctaveNoiseSynthesizer.h>
#include <src/utilities/ImGui.h>
#include <src/utilities/Log.h>
#include <src/utilities/VectorUtility.h>
#include <src/world/MaterialManager.h>
#include <src/world/VoxelChunkData.h>
#include <tracy/Tracy.hpp>
#include <format>
#include <src/utilities/Log.h>

void PrototypeWorldGenerator::generateData(VoxelChunkData& data)
{
    auto& materialManager = MaterialManager::getInstance();

    std::shared_ptr<Material> stoneMaterial;
    std::shared_ptr<Material> dirtMaterial;
    std::shared_ptr<Material> grassMaterial;
    std::shared_ptr<Material> oakLogMaterial;
    std::shared_ptr<Material> oakLeafMaterial;

    {
        ZoneScopedN("Get Materials");
        WorldUtility::tryGetMaterial("stone", materialManager, stoneMaterial);
        WorldUtility::tryGetMaterial("dirt", materialManager, dirtMaterial);
        WorldUtility::tryGetMaterial("grass", materialManager, grassMaterial);
        WorldUtility::tryGetMaterial("oak_log", materialManager, oakLogMaterial);
        WorldUtility::tryGetMaterial("oak_leaf", materialManager, oakLeafMaterial);    
    }

    // Fill texture data with random noise, each block evaluated once
    FastNoiseLite simplexNoise;
    simplexNoise.SetSeed(seed);
    simplexNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    float stoneFrequency = 0.01;
    // float noise = simplexNoise.GetNoise(localX * stoneFrequency, localY * stoneFrequency, localZ * stoneFrequency);

    siv::BasicPerlinNoise<float> perlinNoise(seed);

    // GridPointSynthesizer pointSynthesizer(seed);
    PoissonDiskPointSynthesizer pointSynthesizer(seed);
    std::vector<glm::vec3> treeLocations;

    {
        ZoneScopedN("Generate points");

        int numPoints = 20;
        pointSynthesizer.generatePoints(treeLocations, numPoints);
        pointSynthesizer.rescalePointsToChunkSize(treeLocations, data);
    
        // Lexicographic sort
        VectorUtility::lexicographicSort(treeLocations);
    }

    int treeIndex = 0;
    glm::vec3 treeLocation(treeLocations.at(treeIndex));
    float probabilityToFill = 0.6;

    // Iterating by block since air has empty voxels that don't need to be filled anyways. Form of mipmapping?
    
    {
        ZoneScopedN("Generate terrain");

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
                    Log::verbose(std::format("Tree Origin Voxel:({:.2f}, {:.2f})", originVoxel.x, originVoxel.y));

                    // Naive seeding. Is there a better way?
                    std::srand(seed + chunkPosition.x + chunkPosition.y * 10 + chunkPosition.z * 100 + originVoxel.x * 11 + originVoxel.y * 11);

                    int treeHeightVoxels = randomBetween(treeHeightRangeMeters.x * voxelsPerMeter, treeHeightRangeMeters.y * voxelsPerMeter);
                    int treeWidthVoxels = randomBetween(treeWidthRangeMeters.x * voxelsPerMeter, treeWidthRangeMeters.y * voxelsPerMeter);
                    int leafWidthX = randomBetween(leafWidthXRangeMeters.x * voxelsPerMeter, leafWidthXRangeMeters.y * voxelsPerMeter);
                    int leafWidthY = randomBetween(leafWidthYRangeMeters.x * voxelsPerMeter, leafWidthYRangeMeters.y * voxelsPerMeter);
                    int leafExtentBelowZ = randomBetween(leafExtentBelowZRangeMeters.x * voxelsPerMeter, leafExtentBelowZRangeMeters.y * voxelsPerMeter);
                    int leafExtentAboveZ = randomBetween(leafExtentAboveZRangeMeters.x * voxelsPerMeter, leafExtentAboveZRangeMeters.y * voxelsPerMeter);

                    
                    TreeStructure tree(oakLogMaterial, oakLeafMaterial, treeHeightVoxels, treeWidthVoxels, leafWidthX, leafWidthY, leafExtentBelowZ, leafExtentAboveZ, leafProbabilityToFill);
                    tree.generate(data, originVoxel);

                    treeIndex++;
                    if (treeIndex < treeLocations.size())
                    {
                        treeLocation = treeLocations.at(treeIndex);
                    }
                    else
                    {
                        treeLocation = { -1, -1, -1 };
                    }
                }
            }
        }
    }
}

int PrototypeWorldGenerator::randomBetween(int min, int max)
{
    return min + rand() % (max - min + 1);
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

            if (ImGui::BeginMenu("Surface Trees"))
            {
                ImGui::DragFloatRange2("Tree Height Range (Meters)", &treeHeightRangeMeters.x, &treeHeightRangeMeters.y);
                ImGui::DragFloatRange2("Tree Width Range (Meters)", &treeWidthRangeMeters.x, &treeWidthRangeMeters.y);
                ImGui::DragFloatRange2("Leaf Width X Range (Meters)", &leafWidthXRangeMeters.x, &leafWidthXRangeMeters.y);
                ImGui::DragFloatRange2("Leaf Width Y Range (Meters)", &leafWidthYRangeMeters.x, &leafWidthYRangeMeters.y);
                ImGui::DragFloatRange2("Leaf Extent Above Range (Meters)", &leafExtentAboveZRangeMeters.x, &leafExtentAboveZRangeMeters.y);
                ImGui::DragFloatRange2("Leaf Extent Below Range (Meters)", &leafExtentBelowZRangeMeters.x, &leafExtentBelowZRangeMeters.y);

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
