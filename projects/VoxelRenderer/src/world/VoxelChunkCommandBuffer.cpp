#include "VoxelChunkCommandBuffer.h"

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

void VoxelChunkCommandBuffer::setEnableCpuMipmaps(bool enableCpuMipmaps)
{
    commands.emplace_back(SetEnableCpuMipmaps, setEnableCpuMipmapsCommands.size());
    setEnableCpuMipmapsCommands.emplace_back(enableCpuMipmaps);
}

void VoxelChunkCommandBuffer::setActiveLod(int activeLod)
{
    commands.emplace_back(SetActiveLod, setActiveLodCommands.size());
    setActiveLodCommands.emplace_back(activeLod);
}

void VoxelChunkCommandBuffer::setMaxLod(int maxLod)
{
    commands.emplace_back(SetMaxLod, setMaxLodCommands.size());
    setMaxLodCommands.emplace_back(maxLod);
}

void VoxelChunkCommandBuffer::apply(const std::shared_ptr<VoxelChunkComponent>& component, const std::shared_ptr<SceneComponent>& scene, std::mutex& gpuUploadMutex) const
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
    // TODO: Note that change tracking also needs to consider the active LOD
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
                if (command.existsOnGpu != component->getExistsOnGpu())
                {
                    if (command.existsOnGpu)
                    {
                        component->allocateGpuData(chunkData.getSize());
                    }
                    else
                    {
                        component->deallocateGpuData();
                    }
                }

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

                        {
                            std::lock_guard lockGpuUpload(gpuUploadMutex);
                            chunkData.copyTo(*component->getChunk());
                        }

                        isGpuUpToDate = true;
                    }

                    lockComponent.lock();
                    if (!component->getIsPartOfWorld())
                    {
                        Log::warning("Failed to apply VoxelChunkCommandBuffer::SetExistsOnGpu command (lock 2). VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

                        return;
                    }
                }

                break;
            }
            case SetEnableCpuMipmaps:
            {
                // TODO

                break;
            }
            case SetActiveLod:
            {
                // TODO

                break;
            }
            case SetMaxLod:
            {
                // TODO

                break;
            }
        }
    }

    if (!isGpuUpToDate && component->getExistsOnGpu())
    {
        ZoneScopedN("VoxelChunkCommandBuffer::apply - Write to GPU");

        {
            std::lock_guard lockGpuUpload(gpuUploadMutex);
            chunkData.copyTo(*component->getChunk());
        }
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
