#include "VoxelChunkComponent.h"

#include <src/Constants.h>

VoxelChunkComponent::VoxelChunkComponent(glm::ivec3 worldSize)
{
    chunk = std::make_shared<VoxelWorld>(Constants::VoxelChunkComponent::chunkSize);
}

std::shared_ptr<VoxelWorld>& VoxelChunkComponent::getChunk()
{
    return chunk;
}
