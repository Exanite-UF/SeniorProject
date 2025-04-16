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

    // Technically, this should call generateData, but generateData is private
    // AND generateData doesn't allow all of the necessary parameters to be specified
    //
    // This is an issue with the WorldGenerator API, but requires a decent refactor
    // We also don't know if we need access to the SceneComponent pointer (we really shouldn't). It's also a cyclic dependency.
    generator.generate(data, scene, false);
}

void VoxelChunkManagerWorldGenerator::showDebugMenu()
{
    // Do nothing
}
