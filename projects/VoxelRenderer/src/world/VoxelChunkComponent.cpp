#include "VoxelChunkComponent.h"

#include <src/Constants.h>

VoxelChunkComponent::VoxelChunkComponent()
{
    std::make_shared<VoxelWorld>(Constants::VoxelChunkComponent::chunkSize, makeNoiseComputeProgram, makeMipMapComputeProgram, assignMaterialComputeProgram))
}
