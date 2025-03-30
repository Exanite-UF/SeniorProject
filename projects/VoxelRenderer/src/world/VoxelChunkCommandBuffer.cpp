#include "VoxelChunkCommandBuffer.h"

#include <algorithm>
#include <ranges>
#include <src/utilities/Log.h>
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
    std::unique_lock lockComponent(component->getMutex());
    if (!component->getIsPartOfWorld())
    {
        Log::warning("Failed to apply VoxelChunkCommandBuffer. VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

        return;
    }

    auto& chunkData = component->getRawChunkData();

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
                auto previouslyExistedOnGpu = component->getExistsOnGpu();

                // Never write to GPU using setExistsOnGpu, we can handle it better here
                component->setExistsOnGpu(command.existsOnGpu, false);

                // Write if needed
                if (command.existsOnGpu && command.writeToGpu)
                {
                    ZoneScopedN("VoxelChunkCommandBuffer::apply - SetExistsOnGpu - Write to GPU");

                    // We only need shared access because we are modifying a GPU resource
                    // Writing takes a while so this is an optimization to prevent acquiring exclusive access for a long time when we don't need it
                    lockComponent.unlock();
                    {
                        std::shared_lock sharedLockComponent(component->getMutex());
                        if (!component->getIsPartOfWorld())
                        {
                            Log::warning("Failed to apply VoxelChunkCommandBuffer::SetExistsOnGpu command (lock 1). VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

                            return;
                        }

                        component->getRawChunkData().writeTo(*component->getChunk());

                        isGpuUpToDate = true;
                    }

                    lockComponent.lock();
                    if (!component->getIsPartOfWorld())
                    {
                        Log::warning("Failed to apply VoxelChunkCommandBuffer::SetExistsOnGpu command (lock 2). VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

                        return;
                    }
                }

                // Check if we need to update list of uploaded chunks
                if (previouslyExistedOnGpu != command.existsOnGpu)
                {
                    ZoneScopedN("VoxelChunkCommandBuffer::apply - SetExistsOnGpu - Update uploaded chunks list");

                    // Lock the scene mutex
                    std::lock_guard lockScene(scene->getMutex());
                    if (command.existsOnGpu)
                    {
                        auto isPartOfScene = std::find(scene->allChunks.begin(), scene->allChunks.end(), component) != scene->allChunks.end();
                        auto isPartOfWorld = component->getIsPartOfWorld();

                        if (isPartOfScene && isPartOfWorld)
                        {
                            scene->uploadedChunks.push_back(component);
                        }
                    }
                    else
                    {
                        std::erase(scene->uploadedChunks, component);
                    }
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
