#pragma once

#include <src/procgen/data/TreeStructure.h>
#include <src/procgen/generators/WorldGenerator.h>
#include <src/procgen/synthesizers/TextureDataSynthesizer.h>

// TODO: Idea: Reduce octaves for farther away positions ~ level of detail
class PrototypeWorldGenerator : public WorldGenerator
{
private:
    std::shared_ptr<TextureDataSynthesizer> textureDataSynthesizer;
    std::shared_ptr<TextureDataA> textureData;

    int seed = 1;

    // Stone Terrain
    int octaves = 3;
    float persistence = 0.5;

    glm::vec2 scale2D = glm::vec2(1 / 8.0);

    float frequency2D = 0.002 / 8; // Scale of the 2D perlin noise
    float frequency3D = 0.01 / 2; // Scale of the 3D perlin noise
    float verticalStride = 1; // Reduces noise checks by resuing occupancy result vertically

    // These could all change continuously with space
    int terrainMaxAmplitude = 200; // Terrain amplitude in voxels
    int baseHeight = 100; // The base height in voxels
    float surfaceProbability = 0.6; // Probablility of a voxel at the surface
    float airProbability = 0.2; // The probablity of a voxel at the maximum height
    float surfaceToBottomFalloffRate = 1; // Fall off from the bottom (higher numbers mean deeper caves)

    // Replace surface with dirt
    int dirtDepth = 3;

    // Replace surface with grass
    int grassDepth = 1;

    int noMoreGrassDepth = 10;

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

    void generateTerrain(VoxelChunkData& data);

    int randomBetween(int min, int max);

    void generate3DSplit(VoxelChunkData& data, glm::ivec4 pos, glm::ivec3 size);
    void generate3DEnd(VoxelChunkData& data, glm::ivec3 pos);

public:
    explicit PrototypeWorldGenerator(const std::shared_ptr<TextureDataSynthesizer>& textureDataSynthesizer);

    void showDebugMenu() override;
};
