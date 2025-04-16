#include "VoxelChunkManagerWorldGenerator.h"

#include <src/procgen/synthesizers/TextureOctaveNoiseSynthesizer.h>

#include "PrototypeWorldGenerator.h"

void VoxelChunkManagerWorldGenerator::generateData(VoxelChunkData& data)
{
    // You can define which generator the VoxelChunkManager should use here
    // This decouples the VoxelChunkManager from the WorldGenerator it should use

    int seed = 0;
    int octaves = 3;
    float persistence = 0.5;
    auto octaveSynthesizer = std::make_shared<TextureOctaveNoiseSynthesizer>(seed, octaves, persistence);

    PrototypeWorldGenerator generator(octaveSynthesizer);
    generator.setChunkSize(getChunkSize());
    generator.setChunkPosition(getChunkPosition());
    generator.generate(data, scene, false);
}

void VoxelChunkManagerWorldGenerator::showDebugMenu()
{
    // Do nothing
}
