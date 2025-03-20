#pragma once

#include <src/procgen/generators/WorldGenerator.h>

class ExaniteWorldGenerator : public WorldGenerator
{
private:
    std::string materialKey = "dirt";

protected:
    void generateData() override;

public:
    explicit ExaniteWorldGenerator(const glm::ivec3& chunkSize);

    void showDebugMenu() override;
};
