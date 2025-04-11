#include <memory>

#include <PerlinNoise/PerlinNoise.hpp>

#include <FastNoiseLite/FastNoiseLite.h>
#include <cstdlib>
#include <format>
#include <src/procgen/ChunkHierarchyManager.h>
#include <src/procgen/PrintUtility.h>
#include <src/procgen/WorldUtility.h>
#include <src/procgen/data/FlatArrayData.h>
#include <src/procgen/generators/PrototypeWorldGenerator.h>
#include <src/procgen/synthesizers/GridPointSynthesizer.h>
#include <src/procgen/synthesizers/PoissonDiskPointSynthesizer.h>
#include <src/procgen/synthesizers/TextureOctaveNoiseSynthesizer.h>
#include <src/utilities/ImGui.h>
#include <src/utilities/Log.h>
#include <src/utilities/VectorUtility.h>
#include <src/world/MaterialManager.h>
#include <src/world/VoxelChunkData.h>
#include <tracy/Tracy.hpp>

void PrototypeWorldGenerator::generateData(VoxelChunkData& data)
{
    auto& chunkHierarchyManager = ChunkHierarchyManager::getInstance();
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

    {
        ZoneScopedN("Generate terrain");

        siv::BasicPerlinNoise<float> perlinNoise(seed);

        // TODO: Decorate with grass. Data dependency: terrain z-value, grass spawns on surface. Extra computation.
        // Intended solution: store surface to texture. Modify update texture. Caching step.
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
            }
        }

        {
            ZoneScopedN("Generate trees");

            // Decoration: Create trees by searching points. 20 trees vs 512^3 checks + caching
            // TODO: Find precise lock locations
            if (!chunkHierarchyManager.isChunkGenerated(chunkPosition, 0))
            {
                // GridPointSynthesizer pointSynthesizer(seed);
                PoissonDiskPointSynthesizer pointSynthesizer(seed);
                std::vector<glm::vec3> treeLocations;
                std::vector<TreeStructure> treeStructures;

                {
                    ZoneScopedN("Generate points");

                    int numPoints = 20;
                    pointSynthesizer.generatePoints(treeLocations, numPoints);
                    pointSynthesizer.rescalePointsToChunkSize(treeLocations, data);

                    // Lexicographic sort
                    // VectorUtility::lexicographicSort(treeLocations);
                }

                for (int i = 0; i < treeLocations.size(); i++)
                {
                    glm::ivec2 originVoxel(treeLocations[i].x, treeLocations[i].y);
                    TreeStructure tree = createRandomTreeInstance(data, chunkPosition, originVoxel, seed, oakLogMaterial, oakLeafMaterial);
                    chunkHierarchyManager.addStructure(chunkPosition, data, originVoxel, tree);
                }

                chunkHierarchyManager.setChunkGenerated(chunkPosition, 0, true);
            }
        }

        {
            ZoneScopedN("Chunk Hierarchy Draw Structures");

            // Chunk hierarchy manager is a 'cache'.
            auto structures = chunkHierarchyManager.getStructuresFromChunk(chunkPosition);

            // Raycast down, place on surface.
            for (auto& structure : structures)
            {
                const glm::ivec2& originXY = structure->structure.getOriginVoxel();

                int finalZ = 1;
                for (int z = data.getSize().z - 1; z > 0; z--)
                {
                    glm::ivec3 position(originXY.x, originXY.y, z);
                    if (data.getVoxelOccupancy(position))
                    {
                        finalZ = z;
                        break;
                    }
                }

                // TODO: Raycast here
                structure->structure.generate(data, finalZ);
            }
        }
    }
}

int PrototypeWorldGenerator::randomBetween(int min, int max)
{
    return min + rand() % (max - min + 1);
}

TreeStructure PrototypeWorldGenerator::createRandomTreeInstance(VoxelChunkData& chunkData, glm::ivec3 chunkPosition, glm::ivec2 originVoxel, int seed, std::shared_ptr<Material>& logMaterial, std::shared_ptr<Material>& leafMaterial)
{
    std::srand(seed + chunkPosition.x + chunkPosition.y * 10 + chunkPosition.z * 100 + originVoxel.x * 11 + originVoxel.y * 11);

    int treeHeightVoxels = randomBetween(treeHeightRangeMeters.x * voxelsPerMeter, treeHeightRangeMeters.y * voxelsPerMeter);
    int treeWidthVoxels = randomBetween(treeWidthRangeMeters.x * voxelsPerMeter, treeWidthRangeMeters.y * voxelsPerMeter);
    int leafWidthX = randomBetween(leafWidthXRangeMeters.x * voxelsPerMeter, leafWidthXRangeMeters.y * voxelsPerMeter);
    int leafWidthY = randomBetween(leafWidthYRangeMeters.x * voxelsPerMeter, leafWidthYRangeMeters.y * voxelsPerMeter);
    int leafExtentBelowZ = randomBetween(leafExtentBelowZRangeMeters.x * voxelsPerMeter, leafExtentBelowZRangeMeters.y * voxelsPerMeter);
    int leafExtentAboveZ = randomBetween(leafExtentAboveZRangeMeters.x * voxelsPerMeter, leafExtentAboveZRangeMeters.y * voxelsPerMeter);

    TreeStructure tree(originVoxel, logMaterial, leafMaterial, treeHeightVoxels, treeWidthVoxels, leafWidthX, leafWidthY, leafExtentBelowZ, leafExtentAboveZ, leafProbabilityToFill);

    return tree;
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
