#pragma once

#include <src/procgen/TextureDataSynthesizer.h>
#include <src/procgen/WorldGenerator.h>

// TODO: Idea: Reduce octaves for farther away positions ~ level of detail
class TextureHeightmapWorldGenerator : public WorldGenerator
{
private:
    std::shared_ptr<TextureDataSynthesizer> textureDataSynthesizer;
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
