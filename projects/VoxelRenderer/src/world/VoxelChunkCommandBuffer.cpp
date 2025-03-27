#include "VoxelChunkCommandBuffer.h"

#include <tracy/Tracy.hpp>

void VoxelChunkCommandBuffer::setSize(const glm::ivec3& size)
{
    commands.emplace_back(SetSize, setSizeCommands.size());
    setSizeCommands.emplace_back(size);
}

void VoxelChunkCommandBuffer::setVoxelOccupancy(const glm::ivec3& position, bool isOccupied)
{
    commands.emplace_back(SetOccupancy, setOccupancyCommands.size());
    setOccupancyCommands.emplace_back(position, isOccupied);
}

void VoxelChunkCommandBuffer::setVoxelMaterial(const glm::ivec3& position, const std::shared_ptr<Material>& material)
{
    commands.emplace_back(SetMaterial, setMaterialCommands.size());
    setMaterialCommands.emplace_back(position, material->getIndex());
}

void VoxelChunkCommandBuffer::setVoxelMaterialIndex(const glm::ivec3& position, uint16_t materialIndex)
{
    commands.emplace_back(SetMaterial, setMaterialCommands.size());
    setMaterialCommands.emplace_back(position, materialIndex);
}

void VoxelChunkCommandBuffer::clearOccupancyMap()
{
    commands.emplace_back(ClearOccupancy, 0);
}

void VoxelChunkCommandBuffer::clearMaterialMap()
{
    commands.emplace_back(ClearMaterial, 0);
}

void VoxelChunkCommandBuffer::copyFrom(const std::shared_ptr<VoxelChunkData>& data)
{
    commands.emplace_back(Copy, copyCommands.size());
    copyCommands.emplace_back(data);
}

void VoxelChunkCommandBuffer::setExistsOnGpu(const bool existsOnGpu, const bool writeToGpu)
{
    commands.emplace_back(SetExistsOnGpu, setExistsOnGpuCommands.size());
    setExistsOnGpuCommands.emplace_back(existsOnGpu, writeToGpu);
}

void VoxelChunkCommandBuffer::apply(const std::shared_ptr<VoxelChunkComponent>& component, const std::shared_ptr<SceneComponent>& scene) const
{
    ZoneScoped;

    // Acquire lock for component, but not for the scene
    std::lock_guard lockComponent(component->getMutex());

    auto& chunkData = component->getChunkData();

    // TODO: Track exact changes for optimized CPU -> GPU copies
    // Change tracking
    bool isGpuUpToDate = true;

    for (const auto entry : commands)
    {
        switch (entry.type)
        {
            case SetSize:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetSize");

                auto command = setSizeCommands.at(entry.index);
                chunkData.setSize(command.size);

                isGpuUpToDate = false;

                break;
            }
            case SetOccupancy:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetOccupancy");

                auto command = setOccupancyCommands.at(entry.index);
                chunkData.setVoxelOccupancy(command.position, command.isOccupied);

                isGpuUpToDate = false;

                break;
            }
            case SetMaterial:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetMaterial");

                auto command = setMaterialCommands.at(entry.index);
                chunkData.setVoxelMaterialIndex(command.position, command.materialIndex);

                isGpuUpToDate = false;

                break;
            }
            case ClearOccupancy:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - ClearOccupancy");

                chunkData.clearOccupancyMap();

                isGpuUpToDate = false;

                break;
            }
            case ClearMaterial:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - ClearMaterial");

                chunkData.clearMaterialMap();

                isGpuUpToDate = false;

                break;
            }
            case Copy:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - Copy");

                auto command = copyCommands.at(entry.index);
                chunkData.copyFrom(*command.source);

                isGpuUpToDate = false;

                break;
            }
            case SetExistsOnGpu:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetExistsOnGpu");

                auto command = setExistsOnGpuCommands.at(entry.index);
                component->setExistsOnGpu(command.existsOnGpu, command.writeToGpu);

                if (command.writeToGpu)
                {
                    isGpuUpToDate = true;
                }

                break;
            }
        }
    }

    if (!isGpuUpToDate && component->getExistsOnGpu())
    {
        ZoneScopedN("VoxelChunkCommandBuffer::apply - Write to GPU");

        auto& chunk = component->getChunk();
        chunkData.writeTo(*chunk);
    }
}

void VoxelChunkCommandBuffer::clear()
{
    ZoneScoped;

    setSizeCommands.clear();
    setOccupancyCommands.clear();
    setMaterialCommands.clear();
    copyCommands.clear();
}
