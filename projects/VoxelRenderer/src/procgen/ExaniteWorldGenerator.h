#pragma once

#include <src/procgen/WorldGenerator.h>

class ExaniteWorldGenerator : public WorldGenerator
{
private:
    std::string materialId = "dirt";

protected:
    void generateData() override;

public:
    ExaniteWorldGenerator(glm::ivec3 worldSize);

    void showDebugMenu() override;
};
