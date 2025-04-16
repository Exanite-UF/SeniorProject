#include <tracy/Tracy.hpp>

#include <src/procgen/generators/WorldGenerator.h>
#include <src/utilities/MeasureElapsedTimeScope.h>
#include <src/world/VoxelChunkComponent.h>
#include <src/world/VoxelChunkData.h>
#include <src/world/VoxelChunkManager.h>

WorldGenerator::WorldGenerator() = default;

void WorldGenerator::generate(VoxelChunkData& data, const std::shared_ptr<SceneComponent>& scene, const bool clearData)
{
    ZoneScoped;

    if (clearData)
    {
        data.clearOccupancyMap();
        data.clearMaterialMap();
    }

    this->scene = scene;
    generateData(data);
}

void WorldGenerator::generate(const std::shared_ptr<VoxelChunkComponent>& chunk, const std::shared_ptr<SceneComponent>& scene)
{
    ZoneScoped;

    std::lock_guard lock(chunk->getMutex());

    auto data = std::make_shared<VoxelChunkData>(chunk->getRawChunkData().getSize());
    this->scene = scene;
    generateData(*data);

    VoxelChunkCommandBuffer commandBuffer {};
    commandBuffer.copyFrom(data);

    VoxelChunkManager::getInstance().submitCommandBuffer(chunk, commandBuffer);
}

void WorldGenerator::setChunkSize(const glm::ivec3& chunkSize)
{
    this->chunkSize = chunkSize;
}

const glm::ivec3& WorldGenerator::getChunkSize() const
{
    return chunkSize;
}

void WorldGenerator::setChunkPosition(const glm::ivec3& chunkPosition)
{
    this->chunkPosition = chunkPosition;
}

const glm::ivec3& WorldGenerator::getChunkPosition() const
{
    return chunkPosition;
}
