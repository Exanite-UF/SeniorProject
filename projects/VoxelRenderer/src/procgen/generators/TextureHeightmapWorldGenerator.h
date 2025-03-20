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
    TextureHeightmapWorldGenerator(const glm::ivec3& chunkSize, const std::shared_ptr<TextureDataSynthesizer>& textureDataSynthesizer)
        : WorldGenerator(chunkSize)
    {
        this->textureDataSynthesizer = textureDataSynthesizer;
    }
    void showDebugMenu() override;
};
