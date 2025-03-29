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
#include <PoissonDiskGenerator/PoissonGenerator.h>

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

    int numPoints = 20;
    PoissonGenerator::DefaultPRNG PRNG(seed);
    // Generated points between 0-1
    const auto points = PoissonGenerator::generatePoissonPoints(numPoints, PRNG, false); 

    // Scale to chunk size
    std::vector<glm::vec3> treeLocations;
    for(int i = 0; i < points.size(); i++)
    {
        const PoissonGenerator::Point& point = points.at(i);
        treeLocations.push_back(glm::vec3({std::ceil(point.x * data.getSize().x), std::ceil(point.y * data.getSize().y), 0 }));
    }
    
    // Lexicographic sort
    std::sort(treeLocations.begin(), treeLocations.end(), [](const glm::vec3& a, const glm::vec3& b) {
        return a.x < b.x || (a.x == b.x && a.y < b.y); 
    });

    int treeIndex = 0;
    glm::vec3 treeLocation(treeLocations.at(treeIndex));
    float probabilityToFill = 0.6;

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

                int treeHeightVoxels = randomBetween(treeHeightRangeMeters.x * voxelsPerMeter, treeHeightRangeMeters.y * voxelsPerMeter);
                int treeWidthVoxels = randomBetween(treeWidthRangeMeters.x * voxelsPerMeter, treeWidthRangeMeters.y * voxelsPerMeter);
                int leafWidthX = randomBetween(leafWidthXRangeMeters.x * voxelsPerMeter, leafWidthXRangeMeters.y * voxelsPerMeter);
                int leafWidthY = randomBetween(leafWidthYRangeMeters.x * voxelsPerMeter, leafWidthYRangeMeters.y * voxelsPerMeter);
                int leafExtentBelowZ = randomBetween(leafExtentBelowZRangeMeters.x * voxelsPerMeter, leafExtentBelowZRangeMeters.y * voxelsPerMeter);
                int leafExtentAboveZ = randomBetween(leafExtentAboveZRangeMeters.x * voxelsPerMeter, leafExtentAboveZRangeMeters.y * voxelsPerMeter);

                generateTree(data, oakLogMaterial, oakLeafMaterial, originVoxel, treeHeightVoxels, treeWidthVoxels, leafWidthX, leafWidthY, leafExtentBelowZ, leafExtentAboveZ, probabilityToFill); 

                treeIndex++;
                if(treeIndex < treeLocations.size())
                {
                    treeLocation = treeLocations.at(treeIndex);
                } else
                {
                    treeLocation = {-1,-1,-1};
                }
            }
        }
    }
}


int PrototypeWorldGenerator::randomBetween(int min, int max)
{
    return min + rand() % (max - min + 1);
}

void PrototypeWorldGenerator::generateTree(VoxelChunkData& data, std::shared_ptr<Material>& logMaterial, std::shared_ptr<Material>& leafMaterial, glm::vec3 originVoxel, int treeHeightVoxels, int treeWidthVoxels, int leafWidthX, int leafWidthY, int leafExtentBelowZ, int leafExtentAboveZ, float leafProbabilityToFill)
{
    // Tree Trunk
    generateRectangle(data, logMaterial, originVoxel, treeWidthVoxels, treeWidthVoxels, treeHeightVoxels);

    glm::vec3 originOffset = { 0, 0, treeHeightVoxels + 1 };
    originVoxel += originOffset;

    generateAbsPyramid(data, leafMaterial, originVoxel, leafWidthX, leafWidthY, leafExtentAboveZ, leafExtentBelowZ, leafProbabilityToFill);
}

void PrototypeWorldGenerator::generateRectangle(VoxelChunkData& data, std::shared_ptr<Material>& material, glm::vec3 originVoxel, int widthX, int widthY, int height)
{
    int widthXOffset = widthX / 2;
    int widthYOffset = widthY / 2;

    for (int localX = 0; localX <= widthX; ++localX)
    {
        for (int localY = 0; localY <= widthY; ++localY)
        {
            for (int localZ = 0; localZ <= height; ++localZ)
            {
                glm::vec3 localVoxel = { originVoxel.y + localX - widthXOffset, originVoxel.y + localY - widthYOffset, originVoxel.z + localZ };

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
                data.setVoxelMaterial(localVoxel, material);
            }
        }
    }
}

void PrototypeWorldGenerator::generateAbsPyramid(VoxelChunkData& data, std::shared_ptr<Material>& material, glm::vec3 originVoxel, int widthX, int widthY, int extentAboveZ, int extentBelowZ, float probabilityToFill)
{
    int leafWidthRadiusX = widthX / 2;
    int leafWidthRadiusY = widthY / 2;

    // Setup tree function
    int heightZ = extentAboveZ;
    float heightToWidthXRatio = ((float)heightZ) / widthX;
    float heightToWidthYRatio = ((float)heightZ) / widthY;

    for (int localX = -leafWidthRadiusX; localX <= leafWidthRadiusX; ++localX)
    {
        for (int localY = -leafWidthRadiusY; localY <= leafWidthRadiusY; ++localY)
        {
            for (int localZ = -extentBelowZ; localZ <= extentAboveZ; ++localZ)
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

                int treeFunctionSample = heightZ - heightToWidthXRatio * abs(localX) - heightToWidthYRatio * abs(localY);
                // Simple random function. Probably better to clump and also add so it looks more organic.
                bool randomSample = ((float) rand() / RAND_MAX) >= probabilityToFill;

                if (localZ <= treeFunctionSample && randomSample)
                {
                    if (data.getVoxelMaterial(localVoxel) != material)
                    {
                        data.setVoxelOccupancy(localVoxel, true);
                        data.setVoxelMaterial(localVoxel, material);
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
