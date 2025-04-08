#include "VoxelChunkCommandBuffer.h"

#include <src/utilities/Assert.h>
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

void VoxelChunkCommandBuffer::setMaxLod(int maxLod, bool trim)
{
    commands.emplace_back(SetMaxLod, setMaxLodCommands.size());
    setMaxLodCommands.emplace_back(maxLod, trim);
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
    bool shouldExistOnGpu = component->getExistsOnGpu();

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

                // TODO: Update LODs

                break;
            }
            case SetOccupancy:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetOccupancy");

                auto command = setOccupancyCommands.at(entry.index);
                chunkData.setVoxelOccupancy(command.position, command.isOccupied);

                isGpuUpToDate = false;

                // TODO: Update mipmaps
                // TODO: Update LODs

                break;
            }
            case SetMaterial:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetMaterial");

                auto command = setMaterialCommands.at(entry.index);
                chunkData.setVoxelMaterialIndex(command.position, command.materialIndex);

                isGpuUpToDate = false;

                // TODO: Update LODs

                break;
            }
            case ClearOccupancy:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - ClearOccupancy");

                chunkData.clearOccupancyMap();

                isGpuUpToDate = false;

                // TODO: Update LODs

                break;
            }
            case ClearMaterial:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - ClearMaterial");

                chunkData.clearMaterialMap();

                isGpuUpToDate = false;

                // TODO: Update LODs

                break;
            }
            case Copy:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - Copy");

                auto command = copyCommands.at(entry.index);
                chunkData.copyFrom(*command.source);

                isGpuUpToDate = false;

                // TODO: Update LODs

                break;
            }
            case SetExistsOnGpu:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetExistsOnGpu");

                // This command is deferred until the very end of the command buffer execution
                auto command = setExistsOnGpuCommands.at(entry.index);
                shouldExistOnGpu = command.existsOnGpu;

                break;
            }
            case SetEnableCpuMipmaps:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetEnableCpuMipmaps");

                auto command = setEnableCpuMipmapsCommands.at(entry.index);
                chunkData.setHasMipmaps(command.enableCpuMipmaps);

                break;
            }
            case SetActiveLod:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetActiveLod");

                auto command = setActiveLodCommands.at(entry.index);
                Assert::isTrue(component->getChunkManagerData().lods.size() >= command.activeLod, "Requested LOD has not been generated");

                auto previousActiveLod = component->getChunkManagerData().activeLod;
                component->getChunkManagerData().activeLod = command.activeLod;
                component->getTransform()->setLocalScale(glm::vec3(glm::pow(2, command.activeLod)));

                if (previousActiveLod != command.activeLod)
                {
                    isGpuUpToDate = false;
                }

                break;
            }
            case SetMaxLod:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetMaxLod");

                auto command = setMaxLodCommands.at(entry.index);
                auto& lods = component->getChunkManagerData().lods;
                if (lods.size() >= command.maxLod)
                {
                    // We already have enough LODs

                    // Trim if needed
                    if (command.trim)
                    {
                        Assert::isTrue(component->getChunkManagerData().activeLod <= command.maxLod, "LOD is being used, cannot trim");
                        while (lods.size() > command.maxLod)
                        {
                            lods.pop_back();
                        }
                    }

                    // Early exit
                    break;
                }

                // We don't have enough LODs
                // We need to generate them
                while (lods.size() < command.maxLod)
                {
                    lods.push_back(std::make_shared<VoxelChunkData>());
                }

                for (int i = lods.size() + 1; i <= command.maxLod; ++i)
                {
                    auto& previousLod = (i - 1) == 0 ? component->chunkData : *lods.at(i - 2);
                    auto& currentLod = *lods.at(i - 1);

                    previousLod.copyToLod(currentLod);
                }

                break;
            }
        }
    }

    {
        ZoneScopedN("VoxelChunkCommandBuffer::apply - Check if GPU is up to date");

        // This while loop is restricted to only running once
        // The whole point of this is to create a scope that we can break out of since I don't want to split this into a separate function
        while (true)
        {
            if (!shouldExistOnGpu && component->getExistsOnGpu())
            {
                // Deallocate and exit
                component->deallocateGpuData();

                isGpuUpToDate = false;

                break;
            }

            if (isGpuUpToDate)
            {
                // GPU is already up to date, skip writing to the GPU
                break;
            }

            // Write to the GPU
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - Write to GPU");

                // Find active LOD and upload it to the GPU
                // We don't generate the LOD here
                auto activeLodIndex = component->getChunkManagerData().activeLod;
                VoxelChunkData& lod = activeLodIndex == 0 ? component->chunkData : *component->getChunkManagerData().lods.at(activeLodIndex - 1);

                // allocateGpuData is idempotent so we can just call it
                component->allocateGpuData(lod.getSize());

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
                        auto& test = *component->getChunk();
                        lod.copyTo(test);
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
