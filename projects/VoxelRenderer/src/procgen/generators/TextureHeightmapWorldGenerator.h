#pragma once

#include <src/procgen/generators/WorldGenerator.h>
#include <src/procgen/synthesizers/TextureDataSynthesizer.h>

class TextureHeightmapWorldGenerator : public WorldGenerator
{
private:
    std::shared_ptr<TextureDataSynthesizer> textureDataSynthesizer;
    std::shared_ptr<TextureData> textureData;
    float baseHeight = 100;
    glm::ivec3 chunkSize;

    void generateData(VoxelChunkData& data) override;

public:
    explicit TextureHeightmapWorldGenerator(const glm::ivec3& chunkSize, const std::shared_ptr<TextureDataSynthesizer>& textureDataSynthesizer);

    void showDebugMenu() override;
};
