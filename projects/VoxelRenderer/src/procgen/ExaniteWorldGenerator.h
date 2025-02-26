#pragma once

#include <src/procgen/WorldGenerator.h>

class ExaniteWorldGenerator : public WorldGenerator
{
protected:
    void generateData() override;

public:
    ExaniteWorldGenerator(glm::ivec3 worldSize);

    void showDebugMenu() override;
};
