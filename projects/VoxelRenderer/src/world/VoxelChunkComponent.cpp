#include "VoxelChunkComponent.h"

#include <src/Constants.h>

VoxelChunkComponent::VoxelChunkComponent(glm::ivec3 worldSize)
{
    world = std::make_shared<VoxelWorld>(Constants::VoxelChunkComponent::chunkSize);
}
