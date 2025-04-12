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

void VoxelChunkCommandBuffer::setExistsOnGpu(const bool existsOnGpu)
{
    commands.emplace_back(SetExistsOnGpu, setExistsOnGpuCommands.size());
    setExistsOnGpuCommands.emplace_back(existsOnGpu);
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

void VoxelChunkCommandBuffer::clear()
{
    ZoneScoped;

    commands.clear();
    setSizeCommands.clear();
    setOccupancyCommands.clear();
    setMaterialCommands.clear();
    copyCommands.clear();
    setExistsOnGpuCommands.clear();
    setEnableCpuMipmapsCommands.clear();
    setActiveLodCommands.clear();
    setMaxLodCommands.clear();
}

void VoxelChunkCommandBuffer::apply(const std::shared_ptr<VoxelChunkComponent>& component, const std::shared_ptr<SceneComponent>& scene, std::mutex& gpuUploadMutex) const
{
    CommandApplicator applicator(this, component, scene, gpuUploadMutex);
    applicator.apply();
}

VoxelChunkCommandBuffer::CommandApplicator::CommandApplicator(const VoxelChunkCommandBuffer* commandBuffer, const std::shared_ptr<VoxelChunkComponent>& component, const std::shared_ptr<SceneComponent>& scene, std::mutex& gpuUploadMutex)
{
    this->commandBuffer = commandBuffer;
    this->component = component;
    this->scene = scene;

    componentLock = std::move(std::unique_lock(component->getMutex()));
    gpuUploadLock = std::move(std::unique_lock(gpuUploadMutex, std::defer_lock));

    this->shouldExistOnGpu = component->getExistsOnGpu();
}

void VoxelChunkCommandBuffer::CommandApplicator::apply()
{
    ZoneScoped;

    if (!component->getIsPartOfWorld())
    {
        Log::warning("Failed to apply VoxelChunkCommandBuffer. VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

        return;
    }

    auto& chunkData = component->getRawChunkData();
    auto& chunkManagerData = component->getChunkManagerData();

    for (const auto entry : commandBuffer->commands)
    {
        switch (entry.type)
        {
            case SetSize:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetSize");

                auto command = commandBuffer->setSizeCommands.at(entry.index);
                chunkData.setSize(command.size);

                shouldCompletelyWriteToGpu = true;

                shouldCompletelyRegenerateLods = true;
                setActiveLod(chunkManagerData.activeLod);

                break;
            }
            case SetOccupancy:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetOccupancy");

                auto command = commandBuffer->setOccupancyCommands.at(entry.index);
                chunkData.setVoxelOccupancy(command.position, command.isOccupied);

                shouldCompletelyWriteToGpu = true;

                // TODO: Update mipmaps
                // TODO: Update LODs

                break;
            }
            case SetMaterial:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetMaterial");

                auto command = commandBuffer->setMaterialCommands.at(entry.index);
                chunkData.setVoxelMaterialIndex(command.position, command.materialIndex);

                shouldCompletelyWriteToGpu = true;

                // TODO: Update LODs

                break;
            }
            case ClearOccupancy:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - ClearOccupancy");

                chunkData.clearOccupancyMap();

                shouldCompletelyWriteToGpu = true;

                // TODO: Update LODs

                break;
            }
            case ClearMaterial:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - ClearMaterial");

                chunkData.clearMaterialMap();

                shouldCompletelyWriteToGpu = true;

                // TODO: Update LODs

                break;
            }
            case Copy:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - Copy");

                auto command = commandBuffer->copyCommands.at(entry.index);
                chunkData.copyFrom(*command.source);

                shouldCompletelyWriteToGpu = true;

                // TODO: Update LODs

                break;
            }
            case SetExistsOnGpu:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetExistsOnGpu");

                // This command is deferred until the very end of the command buffer execution
                auto command = commandBuffer->setExistsOnGpuCommands.at(entry.index);
                shouldExistOnGpu = command.existsOnGpu;

                shouldCompletelyWriteToGpu = true;

                break;
            }
            case SetEnableCpuMipmaps:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetEnableCpuMipmaps");

                auto command = commandBuffer->setEnableCpuMipmapsCommands.at(entry.index);

                // We only need shared access since only one command buffer is applied at a time per chunk
                // This allows the renderer and other code to keep using the chunk as normal
                componentLock.unlock();
                {
                    std::shared_lock sharedcomponentLock(component->getMutex());
                    if (!component->getIsPartOfWorld())
                    {
                        Log::warning("Failed to apply VoxelChunkCommandBuffer. VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

                        return;
                    }

                    chunkData.setHasMipmaps(command.enableCpuMipmaps);
                }
                componentLock.lock();
                if (!component->getIsPartOfWorld())
                {
                    Log::warning("Failed to apply VoxelChunkCommandBuffer. VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

                    return;
                }

                break;
            }
            case SetActiveLod:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetActiveLod");

                auto command = commandBuffer->setActiveLodCommands.at(entry.index);
                Assert::isTrue(chunkManagerData.requestedMaxLod >= command.activeLod, "Requested LOD has not been generated");

                setActiveLod(command.activeLod);

                break;
            }
            case SetMaxLod:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetMaxLod");

                auto command = commandBuffer->setMaxLodCommands.at(entry.index);
                auto& lods = chunkManagerData.lods;
                if (lods.size() >= command.maxLod)
                {
                    // We already have enough LODs

                    // Trim if needed
                    if (command.trim)
                    {
                        Assert::isTrue(chunkManagerData.activeLod <= command.maxLod, "LOD is being used, cannot trim");
                        while (lods.size() > command.maxLod)
                        {
                            lods.pop_back();
                        }

                        chunkManagerData.requestedMaxLod = command.maxLod;
                    }

                    // Early exit
                    break;
                }

                // We don't have enough LODs
                // We need to generate them
                auto newLodsRequired = command.maxLod - lods.size();
                {
                    // We only need shared access since only one command buffer is applied at a time per chunk
                    // This allows the renderer and other code to keep using the chunk as normal
                    componentLock.unlock();
                    {
                        std::shared_lock sharedComponentLock(component->getMutex());
                        if (!component->getIsPartOfWorld())
                        {
                            Log::warning("Failed to apply VoxelChunkCommandBuffer. VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

                            return;
                        }

                        for (int i = 0; i < newLodsRequired; ++i)
                        {
                            auto& previousLod = lods.size() > 0 ? *lods.at(lods.size() - 1) : component->chunkData;
                            auto previousLodSize = previousLod.getSize();
                            if (previousLodSize.x < 2 || previousLodSize.y < 2 || previousLodSize.z < 2)
                            {
                                // Previous LOD is too small to make another LOD level
                                break;
                            }

                            auto newLod = std::make_shared<VoxelChunkData>();
                            lods.push_back(newLod);

                            previousLod.copyToLod(*newLod);
                        }
                    }
                    componentLock.lock();
                    if (!component->getIsPartOfWorld())
                    {
                        Log::warning("Failed to apply VoxelChunkCommandBuffer. VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

                        return;
                    }
                }

                // Track the max requested LOD even if we don't generate that many
                // This means we "virtually" generate the LOD when the size of the chunk is too small to allow for that many LODs
                // This is to allow users of the command buffer API to ignore the size of the voxel chunk when setting the max and active LODs
                chunkManagerData.requestedMaxLod = command.maxLod;

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

                shouldCompletelyWriteToGpu = true;

                break;
            }

            if (!shouldCompletelyWriteToGpu)
            {
                // GPU is already up to date, skip writing to the GPU
                break;
            }

            // Write to the GPU
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - Write to GPU");

                // Find active LOD and upload it to the GPU
                // We don't generate the LOD here
                auto activeLodIndex = glm::min(chunkManagerData.activeLod, static_cast<int>(chunkManagerData.lods.size()));
                VoxelChunkData& lod = activeLodIndex == 0 ? component->chunkData : *chunkManagerData.lods.at(activeLodIndex - 1);
                auto lodSize = lod.getSize();

                // Break if size is not 0
                if (lodSize.x == 0 || lodSize.y == 0 || lodSize.z == 0)
                {
                    break;
                }

                // TODO: This check shouldn't be needed, but for some reason it is. This hints towards a synchronization issue.
                if (!component->getIsPartOfWorld())
                {
                    Log::error("Failed to apply VoxelChunkCommandBuffer. VoxelChunkComponent is no longer part of the world. This case is unexpected.");

                    return;
                }

                // allocateGpuData is idempotent so we can just call it
                component->allocateGpuData(lod.getSize());

                // We only need shared access because we are modifying a GPU resource
                // Writing takes a while so this is an optimization to prevent acquiring exclusive access for a long time when we don't need it
                componentLock.unlock();
                {
                    std::shared_lock sharedcomponentLock(component->getMutex());
                    if (!component->getIsPartOfWorld())
                    {
                        Log::warning("Failed to apply VoxelChunkCommandBuffer. VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

                        return;
                    }

                    if (!component->getExistsOnGpu())
                    {
                        // Note that this case cannot happen when the VoxelChunkComponent is only modified
                        // by the chunk modification threads.
                        //
                        // This is because the chunk modification threads respect command buffer submission order.
                        Log::error("Failed to apply VoxelChunkCommandBuffer::SetExistsOnGpu command (lock 1). VoxelChunkComponent is no longer uploaded to the GPU. This usually indicates a synchronization error.");

                        return;
                    }

                    {
                        std::lock_guard lockGpuUpload(gpuUploadLock);
                        auto& test = *component->getChunk();
                        lod.copyTo(test);
                    }
                }
                componentLock.lock();
                if (!component->getIsPartOfWorld())
                {
                    Log::warning("Failed to apply VoxelChunkCommandBuffer. VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

                    return;
                }
            }

            break;
        }
    }
}

void VoxelChunkCommandBuffer::CommandApplicator::setActiveLod(int activeLod)
{
    auto& chunkManagerData = component->getChunkManagerData();

    auto previousActiveLod = chunkManagerData.activeLod;
    auto lodToActivate = glm::min(activeLod, static_cast<int>(chunkManagerData.lods.size()));

    chunkManagerData.requestedActiveLod = activeLod;
    chunkManagerData.activeLod = lodToActivate;
    component->getTransform()->setLocalScale(glm::vec3(glm::pow(2, lodToActivate)));

    if (previousActiveLod != activeLod)
    {
        shouldCompletelyWriteToGpu = true;
    }
}
