#pragma once

#include <src/procgen/WorldGenerator.h>

class ExampleWorldGenerator : public WorldGenerator
{
private:
    std::string materialKey = "dirt";

protected:
    void generateData() override;

public:
    ExampleWorldGenerator(glm::ivec3 worldSize);

    void showDebugMenu() override;
};
