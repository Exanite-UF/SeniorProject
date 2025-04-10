#include "VoxelChunkComponent.h"

#include <src/Constants.h>
#include <src/utilities/MeasureElapsedTimeScope.h>
#include <tracy/Tracy.hpp>

VoxelChunkComponent::VoxelChunkComponent()
    : VoxelChunkComponent(false)
{
}

VoxelChunkComponent::VoxelChunkComponent(const bool shouldGeneratePlaceholderData)
{
    ZoneScoped;

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

const VoxelChunkData& VoxelChunkComponent::getChunkData()
{
    return chunkData;
}

std::shared_mutex& VoxelChunkComponent::getMutex()
{
    return mutex;
}

VoxelChunkData& VoxelChunkComponent::getRawChunkData()
{
    return chunkData;
}

VoxelChunkComponent::RendererData& VoxelChunkComponent::getRendererData()
{
    return rendererData;
}

VoxelChunkComponent::ChunkManagerData& VoxelChunkComponent::getChunkManagerData()
{
    return modificationData;
}

bool VoxelChunkComponent::getExistsOnGpu() const
{
    return existsOnGpu;
}

void VoxelChunkComponent::allocateGpuData(const glm::ivec3& size)
{
    ZoneScoped;

    assertIsPartOfWorld();

    existsOnGpu = true;

    if (!chunk.has_value())
    {
        chunk = std::make_unique<VoxelChunk>(size, false);
    }
    else
    {
        chunk.value()->setSize(size);
    }
}

void VoxelChunkComponent::deallocateGpuData()
{
    ZoneScoped;

    existsOnGpu = false;
    if (chunk.has_value())
    {
        chunk.value().reset();
    }
}

void VoxelChunkComponent::onRemovingFromWorld()
{
    ZoneScoped;

    std::lock_guard lock(getMutex());

    deallocateGpuData();

    Component::onRemovingFromWorld();
}
