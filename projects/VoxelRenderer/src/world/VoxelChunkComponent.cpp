#include "VoxelChunkComponent.h"

#include <src/Constants.h>

VoxelChunkComponent::VoxelChunkComponent()
    : VoxelChunkComponent(false)
{
}

VoxelChunkComponent::VoxelChunkComponent(bool shouldGeneratePlaceholderData)
{
    if (shouldGeneratePlaceholderData)
    {
        isDisplayed = true;
        chunk = std::make_unique<VoxelChunk>(Constants::VoxelChunkComponent::chunkSize, shouldGeneratePlaceholderData);
        chunkData.copyFrom(*chunk.value());
    }
}

const std::unique_ptr<VoxelChunk>& VoxelChunkComponent::getChunk()
{
    return chunk.value();
}

bool VoxelChunkComponent::getIsDisplayed() const
{
    return isDisplayed;
}

void VoxelChunkComponent::setIsDisplayed(const bool isDisplayed)
{
    if (this->isDisplayed == isDisplayed)
    {
        return;
    }

    this->isDisplayed = isDisplayed;

    if (isDisplayed)
    {
        chunk.value().reset();
    }
    else
    {
        chunk = std::make_unique<VoxelChunk>(chunkData.getSize(), false);
        chunkData.writeTo(*chunk.value());
    }
}
