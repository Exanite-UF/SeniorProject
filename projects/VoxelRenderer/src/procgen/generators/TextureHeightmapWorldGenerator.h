#pragma once

#include <src/procgen/generators/WorldGenerator.h>
#include <src/procgen/synthesizers/TextureDataSynthesizer.h>

class TextureHeightmapWorldGenerator : public WorldGenerator
{
private:
    std::shared_ptr<TextureDataSynthesizer> textureDataSynthesizer;
    std::shared_ptr<TextureData> textureData;
    float baseHeight = 100;

    void generateData(VoxelChunkData& data) override;

public:
    explicit TextureHeightmapWorldGenerator(const std::shared_ptr<TextureDataSynthesizer>& textureDataSynthesizer);

    void showDebugMenu() override;
};
