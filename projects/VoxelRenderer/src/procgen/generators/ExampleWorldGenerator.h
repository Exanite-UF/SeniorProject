#pragma once

#include <src/procgen/generators/WorldGenerator.h>

class ExampleWorldGenerator : public WorldGenerator
{
private:
    std::string materialKey = "dirt";

protected:
    void generateData(VoxelChunkData& data) override;

public:
    void showDebugMenu() override;
};
