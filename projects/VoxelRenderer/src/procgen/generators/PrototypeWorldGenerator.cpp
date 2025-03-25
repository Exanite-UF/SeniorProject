#include <memory>

#include <PerlinNoise/PerlinNoise.hpp>

#include <FastNoiseLite/FastNoiseLite.h>
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

    glm::vec3 treeLocation({data.getSize().x/2, data.getSize().y/2, 0});
    int voxelsPerMeter = 8;
    int treeHeightVoxels = 7 * voxelsPerMeter;
    int treeWidthVoxels = 1 * voxelsPerMeter;

    int leafWidthX = 4 * voxelsPerMeter;
    int leafWidthY = 4 * voxelsPerMeter;
    int leafWidthExtentBelowZ = 1 * voxelsPerMeter;
    int leafWidthExtentAboveZ = 4 * voxelsPerMeter;

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
                data.setVoxelOccupancy({x, y, z}, true);
                data.setVoxelMaterial({x, y, z}, stoneMaterial);
            }

            // Replace surface with grass
            int lastHeightBlocks = heightVoxels - 1;
            for (int z = lastHeightBlocks; z >= lastHeightBlocks - grassDepth && z >= 0; --z)
            {
                data.setVoxelOccupancy({x, y, z}, true);
                data.setVoxelMaterial({x, y, z}, grassMaterial);
            }
            lastHeightBlocks -= grassDepth;

            // Replace surface with dirt
            for (int z = lastHeightBlocks; z >= lastHeightBlocks - dirtDepth && z >= 0; --z)
            {
                data.setVoxelOccupancy({x, y, z}, true);
                data.setVoxelMaterial({x, y, z}, dirtMaterial);
            }
            lastHeightBlocks -= dirtDepth;

            if(treeLocation.x == x && treeLocation.y == y)
            {
                glm::vec3 startVoxel = {x, y, heightVoxels};
                
                // Tree Trunk
                int treeWidthRadius = treeWidthVoxels/2;
                
                for(int localX = -treeWidthRadius; localX <= treeWidthRadius; ++localX)
                {
                    for(int localY = -treeWidthRadius; localY <= treeWidthRadius; ++localY)
                    {
                        for(int localZ = 0; localZ <= treeHeightVoxels; ++localZ)
                        {
                            glm::vec3 localVoxel = {startVoxel.y + localX, startVoxel.y + localY, startVoxel.z + localZ};
                            
                            // Fall through
                            if(localVoxel.x <= 0 || localVoxel.y <= 0 || localVoxel.z <= 0)
                            {
                                continue;
                            }

                            if(localVoxel.x > data.getSize().x || localVoxel.y > data.getSize().y || localVoxel.z > data.getSize().z)
                            {
                                continue;
                            }

                            data.setVoxelOccupancy(localVoxel, true);
                            data.setVoxelMaterial(localVoxel, oakLogMaterial);
                        }
                    }
                }

                glm::vec3 startOffset = {0, 0, treeHeightVoxels + 1};
                startVoxel += startOffset;

                int leafWidthRadiusX = leafWidthX/2;
                int leafWidthRadiusY = leafWidthY/2;

                for(int localX = -leafWidthRadiusX; localX <= leafWidthRadiusX; ++localX)
                {
                    for(int localY = -leafWidthRadiusY; localY <= leafWidthRadiusY; ++localY)
                    {
                        for(int localZ = -leafWidthExtentBelowZ; localZ <= leafWidthExtentAboveZ; ++localZ)
                        {
                            glm::vec3 localVoxel = {startVoxel.y + localX, startVoxel.y + localY, startVoxel.z + localZ};
                            
                            // Fall through
                            if(localVoxel.x <= 0 || localVoxel.y <= 0 || localVoxel.z <= 0)
                            {
                                continue;
                            }

                            if(localVoxel.x > data.getSize().x || localVoxel.y > data.getSize().y || localVoxel.z > data.getSize().z)
                            {
                                continue;
                            }

                            if(data.getVoxelMaterial(localVoxel) != oakLogMaterial){
                                data.setVoxelOccupancy(localVoxel, true);
                                data.setVoxelMaterial(localVoxel, oakLogMaterial);
                            }
                        }
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
