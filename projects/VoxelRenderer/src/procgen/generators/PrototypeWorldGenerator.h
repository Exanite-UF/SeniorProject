#pragma once

#include <src/procgen/generators/WorldGenerator.h>
#include <src/procgen/synthesizers/TextureDataSynthesizer.h>

// TODO: Idea: Reduce octaves for farther away positions ~ level of detail
class PrototypeWorldGenerator : public WorldGenerator
{
private:
    std::shared_ptr<TextureDataSynthesizer> textureDataSynthesizer;
    std::shared_ptr<TextureData> textureData;

    glm::ivec3 chunkSize;
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

    void generateData(VoxelChunkData& data) override;

public:
    explicit PrototypeWorldGenerator(const glm::ivec3& chunkSize, const std::shared_ptr<TextureDataSynthesizer>& textureDataSynthesizer);

    void showDebugMenu() override;
};
