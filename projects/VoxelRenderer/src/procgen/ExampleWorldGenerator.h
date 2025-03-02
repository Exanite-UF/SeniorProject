#pragma once

#include <src/procgen/WorldGenerator.h>

class ExampleWorldGenerator : public WorldGenerator
{
private:
    std::string materialId = "dirt";

protected:
    void generateData() override;

public:
    ExampleWorldGenerator(glm::ivec3 worldSize);

    void showDebugMenu() override;
};
