#pragma once

#include <src/procgen/generators/WorldGenerator.h>
#include <src/procgen/synthesizers/TextureDataSynthesizer.h>

// TODO: Idea: Reduce octaves for farther away positions ~ level of detail
class TextureHeightmapWorldGenerator : public WorldGenerator
{
private:
    std::shared_ptr<TextureDataSynthesizer> textureDataSynthesizer;
    std::shared_ptr<TextureData> textureData;
    float baseHeight = 100;

    void generateData() override;

public:
    TextureHeightmapWorldGenerator(glm::ivec3 worldSize, std::shared_ptr<TextureDataSynthesizer> textureDataSynthesizer)
        : WorldGenerator(worldSize)
    {
        this->textureDataSynthesizer = textureDataSynthesizer;
    }
    void showDebugMenu() override;
};
