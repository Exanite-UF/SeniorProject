#pragma once

#include <src/procgen/data/TreeStructure.h>
#include <src/procgen/generators/WorldGenerator.h>
#include <src/procgen/synthesizers/TextureDataSynthesizer.h>

// TODO: Idea: Reduce octaves for farther away positions ~ level of detail
class PrototypeWorldGenerator : public WorldGenerator
{
private:
    std::shared_ptr<TextureDataSynthesizer> textureDataSynthesizer;
    std::shared_ptr<TextureData> textureData;

    int seed = 1;

    // Stone Terrain
    int octaves = 3;
    float persistence = 0.5;
    int baseHeightBlocks = 20;
    float frequency = 0.002;
    int terrainMaxAmplitudeBlocks = 20;

    // Replace surface with dirt
    int dirtDepth = 5;

    // Replace surface with grass
    int grassDepth = 1;

    // Trees
    int voxelsPerMeter = 8;
    glm::vec2 treeHeightRangeMeters = { 5, 7 };
    glm::vec2 treeWidthRangeMeters = { 1, 2 };
    glm::vec2 leafWidthXRangeMeters = { 4, 6 };
    glm::vec2 leafWidthYRangeMeters = { 4, 6 };
    glm::vec2 leafExtentAboveZRangeMeters = { 4, 10 };
    glm::vec2 leafExtentBelowZRangeMeters = { 0, 0 };
    float leafProbabilityToFill = 0.6;

    TreeStructure createRandomTreeInstance(VoxelChunkData& chunkData, glm::ivec3 chunkPosition, glm::ivec2 originVoxel, int seed, std::shared_ptr<Material>& logMaterial, std::shared_ptr<Material>& leafMaterial);
    void generateData(VoxelChunkData& data) override;

    int randomBetween(int min, int max);

public:
    explicit PrototypeWorldGenerator(const std::shared_ptr<TextureDataSynthesizer>& textureDataSynthesizer);

    void showDebugMenu() override;
};
