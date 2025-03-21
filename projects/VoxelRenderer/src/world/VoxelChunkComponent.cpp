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
        chunk = std::make_shared<VoxelChunk>(Constants::VoxelChunkComponent::chunkSize, shouldGeneratePlaceholderData);
        chunkData.copyFrom(*chunk.value());
    }
}

const std::shared_ptr<VoxelChunk>& VoxelChunkComponent::getChunk()
{
    return chunk.value();
}

bool VoxelChunkComponent::getIsDisplayed() const
{
    return isDisplayed;
}

void VoxelChunkComponent::setIsDisplayed(bool isDisplayed)
{
    this->isDisplayed = isDisplayed;
}
