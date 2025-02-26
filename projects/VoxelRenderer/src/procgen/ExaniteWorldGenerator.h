#pragma once

#include <src/procgen/WorldGenerator.h>

class ExaniteWorldGenerator : public WorldGenerator
{
private:
    int materialToUse = 0;

protected:
    void generateData() override;

public:
    ExaniteWorldGenerator(glm::ivec3 worldSize);

    void showDebugMenu() override;
};
