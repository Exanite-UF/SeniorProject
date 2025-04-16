#include "VoxelChunkManagerWorldGenerator.h"

#include <src/procgen/synthesizers/TextureOctaveNoiseSynthesizer.h>

#include "ExaniteWorldGenerator.h"
#include "PrototypeWorldGenerator.h"

void VoxelChunkManagerWorldGenerator::generateData(VoxelChunkData& data)
{
    // You can define which generator the VoxelChunkManager should use here
    // This decouples the VoxelChunkManager from the WorldGenerator it should use

    constexpr int activeGenerator = 0;

    switch (activeGenerator)
    {
        default:
        case 0:
        {
            int seed = 0;
            int octaves = 3;
            float persistence = 0.5;
            auto octaveSynthesizer = std::make_shared<TextureOctaveNoiseSynthesizer>(seed, octaves, persistence);

            PrototypeWorldGenerator generator(octaveSynthesizer);
            generator.setChunkSize(getChunkSize());
            generator.setChunkPosition(getChunkPosition());

            // Technically, this should call generateData, but generateData doesn't allow all of the necessary parameters to be specified
            //
            // This is an issue with the WorldGenerator API, but requires a decent refactor
            // We also don't know if we need access to the SceneComponent pointer (we really shouldn't). It's also a cyclic dependency.
            generator.generate(data, scene, false);

            break;
        }
        case 1:
        {
            ExaniteWorldGenerator generator {};
            generator.setChunkSize(getChunkSize());
            generator.setChunkPosition(getChunkPosition());

            generator.generateData(data);

            break;
        }
    }
}

void VoxelChunkManagerWorldGenerator::showDebugMenu()
{
    // Do nothing
}
