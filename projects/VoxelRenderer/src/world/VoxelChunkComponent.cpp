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

std::shared_mutex& VoxelChunkComponent::getMutex()
{
    return mutex;
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

    std::lock_guard lock(getMutex());

    setExistsOnGpu(false);
}
