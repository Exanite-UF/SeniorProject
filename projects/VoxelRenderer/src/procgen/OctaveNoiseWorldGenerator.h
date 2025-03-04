#pragma once

#include <src/procgen/WorldGenerator.h>

// TODO: Idea: Reduce octaves for farther away positions ~ level of detail
class OctaveNoiseWorldGenerator : public WorldGenerator
{
private:
    float seed = 0;
    float baseHeight = 100;
    int octaves = 3;
    float persistence = 0.5;

    void generateData() override;

public:
    OctaveNoiseWorldGenerator(glm::ivec3 worldSize)
        : WorldGenerator(worldSize)
    {
    }
    void showDebugMenu() override;
};
