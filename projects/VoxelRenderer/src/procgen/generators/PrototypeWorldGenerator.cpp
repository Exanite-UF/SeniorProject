#include "PrototypeWorldGenerator.h"

#include <memory>
#include <vector>
#include <unordered_map>
#include <algorithm>

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

#include <glm/glm.hpp>

void PrototypeWorldGenerator::generateData(VoxelChunkData& data)
{
    auto& chunkHierarchyManager = ChunkHierarchyManager::getInstance();
    auto& materialManager = MaterialManager::getInstance();

    std::shared_ptr<Material> stoneMaterial;
    std::shared_ptr<Material> dirtMaterial;
    std::shared_ptr<Material> grassMaterial;
    std::shared_ptr<Material> oakLogMaterial;
    std::shared_ptr<Material> oakLeafMaterial;
    std::shared_ptr<Material> plasterMaterial;
    std::vector<std::shared_ptr<Material>> lights;

    {
        ZoneScopedN("Get Materials");
        WorldUtility::tryGetMaterial("stone", materialManager, stoneMaterial);
        WorldUtility::tryGetMaterial("dirt", materialManager, dirtMaterial);
        WorldUtility::tryGetMaterial("grass", materialManager, grassMaterial);
        WorldUtility::tryGetMaterial("oak_log", materialManager, oakLogMaterial);
        WorldUtility::tryGetMaterial("oak_leaf", materialManager, oakLeafMaterial);

        WorldUtility::tryGetMaterial("plaster", materialManager, plasterMaterial);

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

    // Fill texture data with random noise, each block evaluated once
    FastNoiseLite simplexNoise;
    simplexNoise.SetSeed(seed);
    simplexNoise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    float stoneFrequency = 0.01;
    // float noise = simplexNoise.GetNoise(localX * stoneFrequency, localY * stoneFrequency, localZ * stoneFrequency);

    //parameters that control how fast the density falls off below the surface
    float a = 1 - std::exp(-surfaceToBottomFalloffRate);
    float b = -surfaceProbability / (surfaceProbability - 1) * a;
    float c = std::log(surfaceProbability * (1 - a));

    //Used to calcualted probability above the surface
    float d = std::log(airProbability / surfaceProbability);

    {
        ZoneScopedN("Generate terrain");

        siv::BasicPerlinNoise<float> perlinNoise(seed);
        siv::BasicPerlinNoise<float> perlinNoise2(seed + 1);

        std::vector<std::vector<std::vector<float>>> cache3D;//This cache is the 3D perlin noise values of the regions that could have terrain (under the max height)


        glm::vec2 offset = chunkSize * chunkPosition;
        for(int ix = 0; ix <= data.getSize().x / stride; ix++){
            int x = stride * ix;
            cache3D.emplace_back();
            for(int iy = 0; iy <= data.getSize().y / stride; iy++){
                int y = stride * iy;
                cache3D.back().emplace_back();
                float perlinNoiseSample = perlinNoise.octave2D_01((x + offset.x) * frequency, (y + offset.y) * frequency, octaves, persistence);
                float maxHeight = std::floor((baseHeight + perlinNoiseSample * terrainMaxAmplitude) / stride) * stride + stride;

                for(int iz = 0; iz <= data.getSize().z / stride; iz++){
                    int z = stride * iz;
                    cache3D.back().back().emplace_back();

                    if(z > maxHeight) continue;//Don't fill out parts of the caches that will never be used
            
                    cache3D[ix][iy][iz] = perlinNoise.octave3D_01((x + offset.x) * frequency3D, (y + offset.y) * frequency3D, (z) * frequency3D, octaves, persistence);
                }
            }
        }

        //generate3DSplit(data, glm::ivec4(0), data.getSize());

        glm::vec3 interpolationFactor3D = glm::vec3(0);
        glm::ivec3 subPos = glm::ivec3(0);
        for(int x = 0; x < data.getSize().x; x++){
            interpolationFactor3D.x = (float)(x % stride) / stride;
            subPos.x = x / stride;
            for(int y = 0; y < data.getSize().y; y++){
                interpolationFactor3D.y = (float)(y % stride) / stride;
                subPos.y = y / stride;

                float perlinNoiseSample = perlinNoise.octave2D_01((x + offset.x) * frequency, (y + offset.y) * frequency, octaves, persistence);
                float perlinNoiseSample2 = perlinNoise2.octave2D_01((x + offset.x) * frequency, (y + offset.y) * frequency, octaves, persistence);
                float maxHeight = baseHeight + perlinNoiseSample * terrainMaxAmplitude;
                float surfaceHeight = baseHeight + perlinNoiseSample * perlinNoiseSample2 * terrainMaxAmplitude;


                for(int z = std::floor((std::min(data.getSize().z - 1, (int)maxHeight)) / verticalStride) * verticalStride; z >= 0; z -= verticalStride){
                    interpolationFactor3D.z = (float)(z % stride) / stride;
                    subPos.z = z / stride;
                    //Smoother step (it doesn't look good)
                    //interpolationFactor = 6.f * glm::pow(interpolationFactor, glm::vec3(5)) - 15.f * glm::pow(interpolationFactor, glm::vec3(4)) + 10.f * glm::pow(interpolationFactor, glm::vec3(3));

                    float random3D = 0;
                    for(int i = 0; i < 2; i++){
                        float temp1 = 0;
                        for(int j = 0; j < 2; j++){
                            float temp2 = 0;
                            for(int k = 0; k < 2; k++){
                                temp2 += (1 - abs(k - interpolationFactor3D.z)) * cache3D[subPos.x + i][subPos.y + j][subPos.z + k];
                            }

                            temp1 += (1 - abs(j - interpolationFactor3D.y)) * temp2;
                        }

                        random3D += (1 - abs(i - interpolationFactor3D.x)) * temp1;
                    }

                    //float random3D = perlinNoise.octave3D_01((x + offset.x) * frequency3D, (y + offset.y) * frequency3D, (z) * frequency3D, octaves, persistence);


                    float p = std::min(1.f, std::exp(d * (float)(z - surfaceHeight) / (maxHeight - surfaceHeight)));
                    if(surfaceProbability < 1){
                        p *= (std::exp(c * z / surfaceHeight) + b) / (1 + b);
                    }

                    if(random3D < p){
                        for(int i = 0; i < verticalStride; i++){
                            data.setVoxelOccupancy({ x, y, z + i }, true);
                        }
                    }
                }
            }
        }
        

        for (int x = 0; x < data.getSize().x; ++x)
        {
            for (int y = 0; y < data.getSize().y; ++y)
            {
                int lastAir = data.getSize().z;
                int maxThick = 0;
                int tempThick = 0;
                for(int z = data.getSize().z - 1; z >= 0; z--){
                    int depth = lastAir - z;

                    bool isUnderground = data.getVoxelOccupancy({ x, y, z });

                    if(isUnderground){
                        tempThick++;
                        if(tempThick > maxThick){
                            maxThick = tempThick;
                        }
                        if(depth <= 3 && maxThick <= 10){
                            data.setVoxelMaterial({ x, y, z }, grassMaterial);
                        }else{
                            if((rand() % 1000) / 1000.0 < 0.25){
                                data.setVoxelMaterial({ x, y, z }, lights.at(rand() % 5));
                            }else{
                                data.setVoxelMaterial({ x, y, z }, plasterMaterial);
                            }
                        }
                    }else{
                        lastAir = z;
                        tempThick = 0;
                    }
                    
                }

                
            }
        }

        if(false){
            ZoneScopedN("Generate trees");

            std::lock_guard lock(chunkHierarchyManager.mutex);

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

        if(false){
            ZoneScopedN("Chunk Hierarchy Draw Structures");

            std::lock_guard lock(chunkHierarchyManager.mutex);

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

void PrototypeWorldGenerator::generate3DSplit(VoxelChunkData& data, glm::ivec4 pos, glm::ivec3 size)
{
    if((1 << pos.w) < std::min({size.x, size.y, size.z})){
        //Not ready yet
        for(int x = 0; x < 2; x++){
            for(int y = 0; y < 2; y++){
                for(int z = 0; z < 2; z++){
                    if((rand() % 1000) / 1000.0 < 0.8){
                        generate3DSplit(data, glm::vec4(2 * pos.x + x, 2 * pos.y + y, 2 * pos.z + z, pos.w + 1), size);
                    }
                }
            }
        }
    }else{
        glm::ivec3 finalPos = glm::ivec3(pos.x, pos.y, pos.z);
        if(glm::all(glm::lessThan(finalPos, size)) && glm::all(glm::greaterThanEqual(finalPos, glm::ivec3(0)))){
            generate3DEnd(data, finalPos);
        }
    }
}

void PrototypeWorldGenerator::generate3DEnd(VoxelChunkData & data, glm::ivec3 pos)
{
    if((rand() % 1000) / 1000.0 < 0.8){
        data.setVoxelOccupancy(pos, true);
        data.setVoxelMaterial(pos, MaterialManager::getInstance().getMaterialByKey("plaster"));
    }
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
