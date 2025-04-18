#include "PrototypeWorldGenerator.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <memory>
#include <unordered_map>
#include <vector>

#include <FastNoise/FastNoise.h>
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
#include <src/world/MaterialManager.h>
#include <src/world/VoxelChunkData.h>
#include <tracy/Tracy.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/integer.hpp>

bool hasGeneratedSeedNode = false;

void PrototypeWorldGenerator::generateData(VoxelChunkData& data)
{
    auto& chunkHierarchyManager = ChunkHierarchyManager::getInstance();
    chunkHierarchyManager.setChunkSize(data.getSize());
    auto& materialManager = MaterialManager::getInstance();

    // Load a set of materials to use
    std::shared_ptr<Material> stoneMaterial;
    std::shared_ptr<Material> dirtMaterial;
    std::shared_ptr<Material> grassMaterial;
    std::shared_ptr<Material> oakLogMaterial;
    std::shared_ptr<Material> oakLeafMaterial;
    std::vector<std::shared_ptr<Material>> lights;

    {
        ZoneScopedN("Get Materials");
        WorldUtility::tryGetMaterial("stone", materialManager, stoneMaterial);
        WorldUtility::tryGetMaterial("dirt", materialManager, dirtMaterial);
        WorldUtility::tryGetMaterial("grass", materialManager, grassMaterial);
        WorldUtility::tryGetMaterial("oak_log", materialManager, oakLogMaterial);
        WorldUtility::tryGetMaterial("oak_leaf", materialManager, oakLeafMaterial);

        lights.emplace_back();
        WorldUtility::tryGetMaterial("white_light", materialManager, lights.back());
        lights.emplace_back();
        WorldUtility::tryGetMaterial("blue_light", materialManager, lights.back());
        lights.emplace_back();
        WorldUtility::tryGetMaterial("red_light", materialManager, lights.back());
        lights.emplace_back();
        WorldUtility::tryGetMaterial("yellow_light", materialManager, lights.back());
        lights.emplace_back();
        WorldUtility::tryGetMaterial("green_light", materialManager, lights.back());
    }

    {
        ZoneScopedN("Generate terrain");

        generateTerrain(data);

        if (true)
        {
            ZoneScopedN("Generate trees");
            std::lock_guard lock(chunkHierarchyManager.mutex);

            std::cout << std::floor((float)chunkPosition.x / chunkSize.x) << " " << std::floor((float)chunkPosition.y / chunkSize.y) << " is being made" << std::endl;

            // Decoration: Create trees by searching points. 20 trees vs 512^3 checks + caching
            // TODO: Find precise lock locations
            // GridPointSynthesizer pointSynthesizer(seed);
            PoissonDiskPointSynthesizer pointSynthesizer(seed);
            std::vector<glm::vec3> treeLocations;
            std::vector<std::shared_ptr<TreeStructure>> treeStructures;
            {
                ZoneScopedN("Generate points");

                int numPoints = 20;
                pointSynthesizer.generatePoints(treeLocations, numPoints);
                pointSynthesizer.rescalePointsToChunkSize(treeLocations, data);
                // Lexicographic sort
                // GeometryUtility::lexicographicSort(treeLocations);
            }

            for (int i = 0; i < treeLocations.size(); i++)
            {
                glm::ivec3 originVoxel((treeLocations[i].x), (treeLocations[i].y), chunkSize.z - 1);

                while (!data.getVoxelOccupancy(originVoxel))
                {
                    originVoxel.z--;
                    if (originVoxel.z == 0)
                    {
                        break;
                    }
                }

                originVoxel.x += chunkPosition.x;
                originVoxel.y += chunkPosition.y;

                // std::cout << originVoxel.x << " " << originVoxel.y << " " << originVoxel.z << std::endl;
                auto tree = createRandomTreeInstance(data, glm::vec3(0), originVoxel, seed, oakLogMaterial, oakLeafMaterial);

                glm::ivec2 distance = tree->getMaxDistanceFromOrigin();

                std::vector<glm::ivec3> boundingBox = {
                    originVoxel + glm::ivec3(distance.x, distance.y, 0),
                    originVoxel + glm::ivec3(distance.x, -distance.y, 0),
                    originVoxel + glm::ivec3(-distance.x, -distance.y, 0),
                    originVoxel + glm::ivec3(-distance.x, distance.y, 0),
                };
                glm::ivec3 temp = glm::ivec3(glm::mod(glm::vec3(originVoxel), glm::vec3(chunkSize)));

                bool circularGeneration = false;
                // tree->getOverlappingChunks()
                // Check if any of those are already generated
                for (int j = 0; j < boundingBox.size(); j++)
                {
                    if (chunkHierarchyManager.isChunkGenerated(boundingBox[j], 0))
                    {
                        circularGeneration = true;
                        break;
                    }
                }

                if (!circularGeneration)
                {
                    chunkHierarchyManager.addStructure(tree->getOriginVoxel(), tree);
                }
            }

            hasGeneratedSeedNode = true;
            chunkHierarchyManager.setChunkGenerated(chunkPosition, 0, true);
        }

        if (true)
        {
            ZoneScopedN("Chunk Hierarchy Draw Structures");

            std::lock_guard lock(chunkHierarchyManager.mutex);

            // Chunk hierarchy manager is a 'cache'.
            auto structures = chunkHierarchyManager.getStructuresForChunk(chunkPosition);
            std::cout << chunkPosition.x << " " << chunkPosition.y << ": " << structures.size() << std::endl;
            // Raycast down, place on surface.
            for (auto& structure : structures)
            {
                glm::ivec3 origin = structure->getOriginVoxel();
                // std::cout << "Initial: " << origin.x << " " << origin.y << " " << origin.z << std::endl;
                origin.x -= chunkPosition.x;
                origin.y -= chunkPosition.y;

                // while (!data.getVoxelOccupancy(origin))
                //{
                //     origin.z--;
                //     if (origin.z == 0)
                //     {
                //         break;
                //     }
                // }

                // std::cout << "Post: " << origin.x << " " << origin.y << " " << origin.z << std::endl;

                glm::ivec3 saved = structure->getOriginVoxel();
                structure->setOriginVoxel(origin);

                // data.setVoxelOccupancy(origin, true);
                // data.setVoxelMaterial(origin, oakLogMaterial);

                // std::cout << structure << std::endl;
                //  TODO: Raycast here
                structure->generate(data);

                structure->setOriginVoxel(saved);
            }
        }
    }
}

void PrototypeWorldGenerator::generateTerrain(VoxelChunkData& data)
{
    ZoneScopedN("Generate terrain");

    // Precalculate some numbers that will be used a lot
    // They don't really have names, they are just components of expressions (The expressions have explainable purposes)

    // parameters that control how fast the density falls off below the surface
    float a = 1 - std::exp(-surfaceToBottomFalloffRate);
    float b = -surfaceProbability / (surfaceProbability - 1) * a;
    float c = std::log(surfaceProbability * (1 - a));

    // Used to calcualted probability above the surface
    float d = std::log(airProbability / surfaceProbability);

    // Load a set of materials to use
    auto& materialManager = MaterialManager::getInstance();

    std::shared_ptr<Material> stoneMaterial;
    std::shared_ptr<Material> dirtMaterial;
    std::shared_ptr<Material> grassMaterial;
    std::shared_ptr<Material> oakLogMaterial;
    std::shared_ptr<Material> oakLeafMaterial;
    std::shared_ptr<Material> limestoneMaterial;
    std::vector<std::shared_ptr<Material>> lights;

    {
        ZoneScopedN("Get Materials");
        WorldUtility::tryGetMaterial("stone", materialManager, stoneMaterial);
        WorldUtility::tryGetMaterial("dirt", materialManager, dirtMaterial);
        WorldUtility::tryGetMaterial("grass", materialManager, grassMaterial);

        WorldUtility::tryGetMaterial("limestone", materialManager, limestoneMaterial);

        lights.emplace_back();
        WorldUtility::tryGetMaterial("white_light", materialManager, lights.back());
        lights.emplace_back();
        WorldUtility::tryGetMaterial("blue_light", materialManager, lights.back());
        lights.emplace_back();
        WorldUtility::tryGetMaterial("red_light", materialManager, lights.back());
        lights.emplace_back();
        WorldUtility::tryGetMaterial("yellow_light", materialManager, lights.back());
        lights.emplace_back();
        WorldUtility::tryGetMaterial("green_light", materialManager, lights.back());
    }

    // The axis scales are different
    FastNoise::New<FastNoise::DomainAxisScale>();
    FastNoise::SmartNode<> source2D = FastNoise::New<FastNoise::Simplex>();
    FastNoise::SmartNode<> source3D = FastNoise::NewFromEncodedNodeTree("JQAAAIA/AAAAPwAAAD8AAIA/CAA="); // For some reason this is the only way to set scale anisotropically

    auto fnFractal = FastNoise::New<FastNoise::FractalFBm>();
    auto fnNormalized = FastNoise::New<FastNoise::Remap>();

    fnFractal->SetOctaveCount(octaves);
    fnFractal->SetGain(0.5);
    fnFractal->SetLacunarity(2.0);

    // Map the value from (-1, 1) to (0, 1)
    fnNormalized->SetSource(fnFractal);
    fnNormalized->SetRemap(-1, 1, 0, 1);

    // auto start = std::chrono::high_resolution_clock::now();

    // Create an array of floats to store the noise output in
    std::vector<float> noiseOutput3D(data.getSize().x * data.getSize().y * data.getSize().z);
    std::vector<float> noiseOutput2D1(data.getSize().x * data.getSize().y);
    std::vector<float> noiseOutput2D2(data.getSize().x * data.getSize().y);

    glm::vec2 offset = chunkPosition;
    {
        ZoneScopedN("Generate Noise");
        fnFractal->SetSource(source2D);
        fnNormalized->GenUniformGrid2D(noiseOutput2D1.data(), offset.y, offset.x, data.getSize().y, data.getSize().x, frequency2D, seed + 1);
        fnNormalized->GenUniformGrid2D(noiseOutput2D2.data(), offset.y, offset.x, data.getSize().y, data.getSize().x, frequency2D, seed + 2);

        fnFractal->SetSource(source3D);
        fnNormalized->GenUniformGrid3D(noiseOutput3D.data(), 0, offset.y, offset.x, data.getSize().z, data.getSize().y, data.getSize().x, frequency3D, seed);
    }

    int index2D1 = 0;
    int index3D = 0;

    glm::ivec3 size = data.getSize();
    // Set the occupancy data of the voxel chunk

    // Assign materials
    // It scans from the top of the voxel chunk
    // When going from air to not air
    //   the first voxel is grass
    //   the second and third are dirt
    //   and everything else is stone
    //   This counter is reset everytime air is hit (unless grass gets disabled)
    //
    // Encountering 10 non-air voxels in a row will disable grass for the rest of the column

    {
        ZoneScopedN("Use Noise");
        for (int x = 0; x < size.x; x++)
        {
            for (int y = 0; y < size.y; y++)
            {

                float perlinNoiseSample = noiseOutput2D1[index2D1];
                float perlinNoiseSample2 = noiseOutput2D2[index2D1++];

                // Calculate the maximum height and surface height
                float maxHeight = baseHeight + perlinNoiseSample * terrainMaxAmplitude;
                float surfaceHeight = baseHeight + perlinNoiseSample * perlinNoiseSample2 * terrainMaxAmplitude;

                int lastAir = data.getSize().z; // track the last height at which we saw air
                int maxThick = 0; // Keep track of the thickest consecutive region we have seen
                int tempThick = 0; // This keeps track of the current number of consecutive non-air voxels

                for (int z = size.z - 1; z >= 0; z--)
                {

                    float random3D = noiseOutput3D[index3D++];

                    // Calculate the threshold for filling in a voxel
                    // It use an formula that happens to give good results
                    // From the surface to the maximum height, the threshold starts at the surface probability and decays exponentially to the air probability
                    // From the z = 0 to the surface, the threshold starts a 1 and decays exponentially to the surface probablity
                    //   The rate of this decays is controlled by surfaceToBottomFalloffRate
                    //   Higher values means deeper caves
                    float p = std::min(1.f, std::exp(d * (float)(z - surfaceHeight) / (maxHeight - surfaceHeight)));
                    if (surfaceProbability < 1)
                    {
                        p *= (std::exp(c * z / surfaceHeight) + b) / (1 + b);
                    }

                    if (z == 0)
                    {
                        p = 1;
                        random3D = 0;
                    }

                    // If the 3D noise at this point is below the threshold then fill the voxel
                    bool isOccupied = false;
                    if (random3D <= p)
                    {
                        isOccupied = true;
                        data.setVoxelOccupancy({ x, y, z }, true);
                    }

                    // Set material
                    {
                        int depth = lastAir - z; // The depth is sensibly, the distance from the last air block

                        bool isUnderground = isOccupied; // Check if we have a non-air voxel

                        if (isUnderground)
                        {

                            // If so, we need to to increment the number of consecutive non-air voxels
                            tempThick++;
                            // And if needed, we should update the thickest region we have seen
                            if (tempThick > maxThick)
                            {
                                maxThick = tempThick;
                            }
                        }
                        else
                        {
                            lastAir = z; // track the last height at which we saw air
                            tempThick = 0; // Reset the consecutive non-air counter

                            // This doesn't have an occupied voxel. It's so that the debug tools have light
                        }

                        // Now we set the material of the voxels based on the description above

                        // Check if grass is enabled
                        if (maxThick <= noMoreGrassDepth)
                        {
                            // If so, try to place grass or dirt
                            if (depth <= grassDepth)
                            {
                                data.setVoxelMaterial({ x, y, z }, grassMaterial);
                                continue;
                            }
                            else if (depth <= dirtDepth)
                            {
                                data.setVoxelMaterial({ x, y, z }, dirtMaterial);
                                continue;
                            }
                        }

                        // The default material is stone

                        // This is stone, I put lights in it for the caves
                        if ((rand() % 1000) / 1000.0 < 0.04)
                        {
                            data.setVoxelMaterial({ x, y, z }, lights.at(rand() % 5)); // Candy lights!
                            continue;
                        }
                        else
                        {
                            data.setVoxelMaterial({ x, y, z }, limestoneMaterial);
                            continue;
                        }
                    }
                }
            }
        }
    }

    // auto end = std::chrono::high_resolution_clock::now();

    // std::cout << std::chrono::duration<double>(end - start).count() << std::endl;
}

int PrototypeWorldGenerator::randomBetween(int min, int max)
{
    return min + rand() % (max - min + 1);
}

void PrototypeWorldGenerator::generate3DSplit(VoxelChunkData& data, glm::ivec4 pos, glm::ivec3 size)
{
    if ((1 << pos.w) < std::min({ size.x, size.y, size.z }))
    {
        // Not ready yet
        for (int x = 0; x < 2; x++)
        {
            for (int y = 0; y < 2; y++)
            {
                for (int z = 0; z < 2; z++)
                {
                    if ((rand() % 1000) / 1000.0 < 0.8)
                    {
                        generate3DSplit(data, glm::vec4(2 * pos.x + x, 2 * pos.y + y, 2 * pos.z + z, pos.w + 1), size);
                    }
                }
            }
        }
    }
    else
    {
        glm::ivec3 finalPos = glm::ivec3(pos.x, pos.y, pos.z);
        if (glm::all(glm::lessThan(finalPos, size)) && glm::all(glm::greaterThanEqual(finalPos, glm::ivec3(0))))
        {
            generate3DEnd(data, finalPos);
        }
    }
}

void PrototypeWorldGenerator::generate3DEnd(VoxelChunkData& data, glm::ivec3 pos)
{
    if ((rand() % 1000) / 1000.0 < 0.8)
    {
        data.setVoxelOccupancy(pos, true);
        data.setVoxelMaterial(pos, MaterialManager::getInstance().getMaterialByKey("plaster"));
    }
}

std::shared_ptr<TreeStructure> PrototypeWorldGenerator::createRandomTreeInstance(VoxelChunkData& chunkData, glm::ivec3 chunkPosition, glm::ivec3 originVoxel, int seed, std::shared_ptr<Material>& logMaterial, std::shared_ptr<Material>& leafMaterial)
{
    std::srand(seed + chunkPosition.x + chunkPosition.y * 10 + chunkPosition.z * 100 + originVoxel.x * 11 + originVoxel.y * 11);

    int treeHeightVoxels = randomBetween(treeHeightRangeMeters.x * voxelsPerMeter, treeHeightRangeMeters.y * voxelsPerMeter);
    int treeWidthVoxels = randomBetween(treeWidthRangeMeters.x * voxelsPerMeter, treeWidthRangeMeters.y * voxelsPerMeter);
    int leafWidthX = randomBetween(leafWidthXRangeMeters.x * voxelsPerMeter, leafWidthXRangeMeters.y * voxelsPerMeter);
    int leafWidthY = randomBetween(leafWidthYRangeMeters.x * voxelsPerMeter, leafWidthYRangeMeters.y * voxelsPerMeter);
    int leafExtentBelowZ = randomBetween(leafExtentBelowZRangeMeters.x * voxelsPerMeter, leafExtentBelowZRangeMeters.y * voxelsPerMeter);
    int leafExtentAboveZ = randomBetween(leafExtentAboveZRangeMeters.x * voxelsPerMeter, leafExtentAboveZRangeMeters.y * voxelsPerMeter);

    // std::cout << "At set: " << originVoxel.x << " " << originVoxel.y << " " << originVoxel.z << std::endl;
    // TODO: Fix this
    // The origin voxel of a tree should be the actual origin, not just 2 of the 3 values
    auto tree = std::shared_ptr<TreeStructure>(new TreeStructure(originVoxel, logMaterial, leafMaterial, treeHeightVoxels, treeWidthVoxels, leafWidthX, leafWidthY, leafExtentBelowZ, leafExtentAboveZ, leafProbabilityToFill));
    // throw std::runtime_error("Not implemented correctly");
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
                ImGui::SliderInt("Base Height", &baseHeight, 0, chunkSize.z);
                ImGui::SliderInt("Octaves", &octaves, 1, 5);
                ImGui::SliderFloat("Persistence", &persistence, 0, 1);
                ImGui::SliderFloat("Frequency", &frequency2D, 0, 1);
                ImGui::SliderInt("Terrain Max Amplitude", &terrainMaxAmplitude, 0, chunkSize.z);

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
