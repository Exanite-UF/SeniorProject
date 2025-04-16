#include "ExaniteWorldGenerator.h"

#include <glm/gtc/integer.hpp>
#include <src/utilities/Assert.h>
#include <src/utilities/ImGui.h>
#include <src/utilities/MeasureElapsedTimeScope.h>
#include <src/world/MaterialManager.h>
#include <tracy/Tracy.hpp>

uint32_t hash(uint32_t value)
{
    value = (value ^ 61) ^ (value >> 16);
    value = value + (value << 3);
    value = value ^ (value >> 4);
    value = value * 0x27d4eb2d;
    value = value ^ (value >> 15);
    return value;
}

uint32_t hash(const glm::ivec2& position)
{
    return hash(position.x) ^ hash(position.y);
}

uint32_t hash(const glm::ivec3& position)
{
    return hash(position.x) ^ hash(position.y) ^ hash(position.z);
}

void ExaniteWorldGenerator::generateData(VoxelChunkData& data)
{
    ZoneScoped;

    auto stoneMaterial = MaterialManager::getInstance().getMaterialByKey("stone");
    auto dirtMaterial = MaterialManager::getInstance().getMaterialByKey("dirt");
    auto grassMaterial = MaterialManager::getInstance().getMaterialByKey("grass");
    auto roadMaterial = MaterialManager::getInstance().getMaterialByKey("asphalt");
    auto roadBorderMaterial = MaterialManager::getInstance().getMaterialByKey("white_light");

    int stoneEnd = 10;
    int dirtEnd = 14;
    int grassEnd = 16;
    int groundHeight = 16;

    for (int z = 0; z < groundHeight; ++z)
    {
        for (int y = 0; y < chunkSize.y; ++y)
        {
            for (int x = 0; x < chunkSize.x; ++x)
            {
                std::shared_ptr<Material> material {};
                if (z < stoneEnd)
                {
                    material = stoneMaterial;
                }
                else if (z < dirtEnd)
                {
                    material = dirtMaterial;
                }
                else if (z < grassEnd)
                {
                    material = grassMaterial;
                }

                if (material)
                {
                    auto position = glm::ivec3(x, y, z);
                    data.setVoxelOccupancy(position, true);
                    data.setVoxelMaterial(position, material);
                }
            }
        }
    }

    int buildingMinHeight = 200;
    int buildingMaxHeight = 400;

    int buildingWidth = 160;
    int spacingWidth = 10;
    int roadWidth = 20;

    int structurePeriod = buildingWidth + spacingWidth * 2 + roadWidth;

    // Generate roads
    for (int y = 0; y < chunkSize.y; ++y)
    {
        for (int x = 0; x < chunkSize.x; ++x)
        {
            auto localPosition = glm::ivec3(x, y, groundHeight);
            auto effectivePosition = glm::ivec2(x, y) + glm::ivec2(chunkPosition);
            effectivePosition = (effectivePosition % structurePeriod + structurePeriod) % structurePeriod;

            auto isRoadX = effectivePosition.x > buildingWidth + spacingWidth && effectivePosition.x < buildingWidth + spacingWidth + roadWidth;
            auto isRoadY = effectivePosition.y > buildingWidth + spacingWidth && effectivePosition.y < buildingWidth + spacingWidth + roadWidth;

            if (isRoadX || isRoadY)
            {
                data.setVoxelOccupancy(localPosition, true);
                data.setVoxelMaterial(glm::ivec3(x, y, groundHeight - 1), roadMaterial);
            }

            auto isRoadBorderX = effectivePosition.x == buildingWidth + spacingWidth || effectivePosition.x == buildingWidth + spacingWidth + roadWidth;
            auto isRoadBorderY = effectivePosition.y == buildingWidth + spacingWidth || effectivePosition.y == buildingWidth + spacingWidth + roadWidth;

            if ((isRoadBorderX || isRoadBorderY) && !(isRoadX || isRoadY))
            {
                data.setVoxelOccupancy(localPosition, true);
                data.setVoxelMaterial(glm::ivec3(x, y, groundHeight), roadBorderMaterial);
            }
        }
    }

    // Generate buildings
    // auto maxBuildingsThatCanFit = glm::ivec2((glm::ivec2(chunkSize) / structurePeriod) + 2);
    // auto worldStartPosition = glm::ivec2((chunkPosition / structurePeriod) * structurePeriod) - structurePeriod;
    // for (int y = 0; y < maxBuildingsThatCanFit.y; ++y)
    // {
    //     for (int x = 0; x < maxBuildingsThatCanFit.x; ++x)
    //     {
    //         auto worldCornerPosition = glm::ivec3(worldStartPosition.x, worldStartPosition.y, 0) + glm::ivec3(structurePeriod * x, structurePeriod * y, groundHeight);
    //         auto localCornerPosition = worldCornerPosition - chunkPosition;
    //
    //         srand(hash(glm::ivec2(worldCornerPosition.x, worldCornerPosition.y)));
    //         int buildingHeight = (rand() % (buildingMaxHeight - buildingMinHeight)) + buildingMinHeight;
    //
    //         generateBuilding(data, localCornerPosition, glm::ivec3(buildingWidth, buildingWidth, buildingHeight));
    //     }
    // }
}

void ExaniteWorldGenerator::showDebugMenu()
{
    ImGui::PushID("ExaniteWorldGenerator");
    {
        if (ImGui::CollapsingHeader("Exanite's Generator (F7)"))
        {
            ImGui::Text("This generator is used by Exanite for testing purposes");
        }
    }
    ImGui::PopID();
}

void ExaniteWorldGenerator::generateBuilding(VoxelChunkData& data, const glm::ivec3& cornerPosition, const glm::ivec3& size)
{
    glm::ivec3 min = cornerPosition;
    min = glm::max(min, glm::ivec3(0));

    glm::ivec3 max = cornerPosition + size;
    max = glm::min(max, chunkSize);

    for (int z = min.z; z < max.z; ++z)
    {
        for (int y = min.y; y < max.y; ++y)
        {
            for (int x = min.x; x < max.x; ++x)
            {
                // The code inside this block represents a single voxel
                glm::ivec3 size2 = data.getSize() >> 4;

                glm::ivec3 position0 = glm::ivec3(x, y, z);
                glm::ivec3 position1 = position0 >> 2;
                glm::ivec3 position2 = position0 >> 4;

                int materialBits0 = ((position0.z & 1) << 2) | ((position0.y & 1) << 1) | ((position0.x & 1) << 0);
                int materialBits1 = ((position1.z & 1) << 2) | ((position1.y & 1) << 1) | ((position1.x & 1) << 0);
                int materialBits2 = ((position2.z & 1) << 2) | ((position2.y & 1) << 1) | ((position2.x & 1) << 0);

                int materialOffset = position2.x + size2.x * (position2.y + size2.y * position2.z);

                int materialIndex = ((materialBits2 << 6) | (materialBits1 << 3) | (materialBits0 << 0)) + materialOffset;

                data.setVoxelOccupancy(position0, true);
                data.setVoxelMaterialIndex(position0, materialIndex % Constants::VoxelChunk::maxMaterialCount);
            }
        }
    }
}
