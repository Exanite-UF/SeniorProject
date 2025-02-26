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
            data.setVoxelOccupancy({ x, y, x }, true);
        }
    }
}

void ExaniteWorldGenerator::showDebugMenu()
{
}
