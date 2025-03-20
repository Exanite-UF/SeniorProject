#pragma once

#include <src/procgen/generators/WorldGenerator.h>
#include <src/procgen/synthesizers/TextureDataSynthesizer.h>

// TODO: Idea: Reduce octaves for farther away positions ~ level of detail
class PrototypeWorldGenerator : public WorldGenerator
{
private:
    std::shared_ptr<TextureDataSynthesizer> textureDataSynthesizer;
    std::shared_ptr<TextureData> textureData;

    int seed = 0;

    // Stone Terrain
    int octaves = 3;
    float persistence = 0.5;
    int baseHeight = 100;
    float frequency = 0.002;
    int terrainMaxAmplitude = 100;

    // Replace surface with dirt
    int dirtDepth = 5;

    // Replace surface with grass
    int grassDepth = 1;

    void generateData() override;

public:
    PrototypeWorldGenerator(glm::ivec3 worldSize, std::shared_ptr<TextureDataSynthesizer> textureDataSynthesizer)
        : WorldGenerator(worldSize)
    {
        this->textureDataSynthesizer = textureDataSynthesizer;
    }
    void showDebugMenu() override;
};
