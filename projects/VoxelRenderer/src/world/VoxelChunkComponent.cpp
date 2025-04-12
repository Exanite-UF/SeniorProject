#include "VoxelChunkComponent.h"

#include <src/Constants.h>
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
    return chunkManagerData;
}

bool VoxelChunkComponent::getExistsOnGpu() const
{
    return chunk.has_value();
}

void VoxelChunkComponent::allocateGpuData(const glm::ivec3& size)
{
    ZoneScoped;

    assertIsPartOfWorld();

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

    if (chunk.has_value())
    {
        chunk.reset();
    }
}

void VoxelChunkComponent::onRemovingFromWorld()
{
    ZoneScoped;

    std::lock_guard lock(getMutex());

    deallocateGpuData();

    Component::onRemovingFromWorld();
}
