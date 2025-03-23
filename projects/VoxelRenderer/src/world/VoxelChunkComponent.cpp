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

const std::unique_ptr<VoxelChunk>& VoxelChunkComponent::getChunkUnsafe()
{
    return chunk.value();
}

std::shared_mutex& VoxelChunkComponent::getChunkMutex()
{
    return chunkMutex;
}

VoxelChunkData& VoxelChunkComponent::getChunkDataUnsafe()
{
    return chunkData;
}

std::mutex& VoxelChunkComponent::getChunkDataMutex()
{
    return chunkDataMutex;
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

    std::lock_guard lockChunk(getChunkMutex());
    std::lock_guard lockChunkData(getChunkDataMutex());

    this->existsOnGpu = existsOnGpu;

    if (existsOnGpu)
    {
        chunk = std::make_unique<VoxelChunk>(chunkData.getSize(), false);
        chunkData.writeTo(*chunk.value());
    }
    else
    {
        if (chunk.has_value())
        {
            chunk.value().reset();
        }
    }
}

void VoxelChunkComponent::onDestroy()
{
    Component::onDestroy();

    setExistsOnGpu(false);
}
