#include "VoxelChunkComponent.h"

#include <src/Constants.h>

VoxelChunkComponent::VoxelChunkComponent(glm::ivec3 worldSize)
{
    chunk = std::make_shared<VoxelChunk>(Constants::VoxelChunkComponent::chunkSize);
}

std::shared_ptr<VoxelChunk>& VoxelChunkComponent::getChunk()
{
    return chunk;
}
