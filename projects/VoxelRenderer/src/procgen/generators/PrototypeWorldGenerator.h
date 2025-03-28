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
    int baseHeightBlocks = 20;
    float frequency = 0.002;
    int terrainMaxAmplitudeBlocks = 20;

    // Replace surface with dirt
    int dirtDepth = 5;

    // Replace surface with grass
    int grassDepth = 1;

    void generateData(VoxelChunkData& data) override;

    int randomBetween(int min, int max);
    void generateTree(VoxelChunkData& data, std::shared_ptr<Material>& logMaterial, std::shared_ptr<Material>& leafMaterial, glm::vec3 originVoxel, int treeHeightVoxels, int treeWidthVoxels, int leafWidthX, int leafWidthY, int leafWidthExtentBelowZ, int leafWidthExtentAboveZ);

public:
    explicit PrototypeWorldGenerator(const std::shared_ptr<TextureDataSynthesizer>& textureDataSynthesizer);

    void showDebugMenu() override;
};
