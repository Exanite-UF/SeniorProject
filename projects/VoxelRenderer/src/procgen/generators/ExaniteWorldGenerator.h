#pragma once

#include <src/procgen/generators/WorldGenerator.h>

class ExaniteWorldGenerator : public WorldGenerator
{
public:
    void generateData(VoxelChunkData& data) override;
    void showDebugMenu() override;

protected:
    void generateBuilding(VoxelChunkData& data, const glm::ivec3& cornerPosition, const glm::ivec3& size);
};
