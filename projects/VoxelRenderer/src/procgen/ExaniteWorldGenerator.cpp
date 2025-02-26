#include "ExaniteWorldGenerator.h"

ExaniteWorldGenerator::ExaniteWorldGenerator(glm::ivec3 worldSize)
    : WorldGenerator(worldSize)
{
}

void ExaniteWorldGenerator::generateData()
{
    for (int x = 0; x < data.getSize().x; ++x)
    {
        for (int y = 0; y < data.getSize().y; ++y)
        {
            data.setVoxelOccupancy(glm::ivec3(x, y, x), true);
            data.setVoxelMaterial(glm::ivec3(x, y, x), 0);
        }
    }
}

void ExaniteWorldGenerator::showDebugMenu()
{
}
