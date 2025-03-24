#include "VoxelChunkComponent.h"

#include <src/Constants.h>
#include <src/utilities/MeasureElapsedTimeScope.h>

VoxelChunkComponent::VoxelChunkComponent()
    : VoxelChunkComponent(false)
{
    MeasureElapsedTimeScope("VoxelChunkComponent constructor", 10);

    chunkData.setSize(Constants::VoxelChunkComponent::chunkSize);
}

VoxelChunkComponent::VoxelChunkComponent(const bool shouldGeneratePlaceholderData)
{
    MeasureElapsedTimeScope("VoxelChunkComponent constructor", 10);

    if (shouldGeneratePlaceholderData)
    {
        existsOnGpu = true;
        chunk = std::make_unique<VoxelChunk>(Constants::VoxelChunkComponent::chunkSize, shouldGeneratePlaceholderData);
        chunkData.copyFrom(*chunk.value());
    }
    else
    {
        chunkData.setSize(Constants::VoxelChunkComponent::chunkSize);
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

void VoxelChunkComponent::setExistsOnGpu(const bool existsOnGpu, const bool writeToGpu)
{
    if (this->existsOnGpu == existsOnGpu)
    {
        return;
    }

    this->existsOnGpu = existsOnGpu;

    if (existsOnGpu)
    {
        chunk = std::make_unique<VoxelChunk>(Constants::VoxelChunkComponent::chunkSize, false);

        if (writeToGpu)
        {
            chunkData.writeTo(*chunk.value());
        }
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
