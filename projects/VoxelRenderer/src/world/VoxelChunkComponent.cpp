#include "VoxelChunkComponent.h"

#include <src/Constants.h>

VoxelChunkComponent::VoxelChunkComponent()
    : VoxelChunkComponent(false)
{
}

VoxelChunkComponent::VoxelChunkComponent(const bool shouldGeneratePlaceholderData)
{
    if (shouldGeneratePlaceholderData)
    {
        existsOnGpu = true;
        chunk = std::make_unique<VoxelChunk>(Constants::VoxelChunkComponent::chunkSize, shouldGeneratePlaceholderData);
        chunkData.copyFrom(*chunk.value());
    }
}

const std::unique_ptr<VoxelChunk>& VoxelChunkComponent::getChunk()
{
    return chunk.value();
}

VoxelChunkData& VoxelChunkComponent::getChunkData()
{
    return chunkData;
}

bool VoxelChunkComponent::getExistsOnGpu() const
{
    return existsOnGpu;
}

void VoxelChunkComponent::setExistsOnGpu(const bool existsOnGpu)
{
    if (this->existsOnGpu == existsOnGpu)
    {
        return;
    }

    this->existsOnGpu = existsOnGpu;

    if (existsOnGpu)
    {
        chunk.value().reset();
    }
    else
    {
        chunk = std::make_unique<VoxelChunk>(chunkData.getSize(), false);
        chunkData.writeTo(*chunk.value());
    }
}

void VoxelChunkComponent::onDestroy()
{
    Component::onDestroy();

    setExistsOnGpu(false);
}
