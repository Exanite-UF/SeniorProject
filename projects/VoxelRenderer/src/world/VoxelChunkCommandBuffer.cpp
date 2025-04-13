#include "VoxelChunkCommandBuffer.h"

#include <glm/gtc/integer.hpp>
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
    // Inputs
    this->commandBuffer = commandBuffer;
    this->component = component;
    this->scene = scene;

    // Synchronization
    componentLock = std::move(std::unique_lock(component->getMutex()));
    gpuUploadLock = std::move(std::unique_lock(gpuUploadMutex, std::defer_lock));

    // Change tracking
    lodRegenerationStartIndex = component->getChunkManagerData().lods.size();

    // Initialize desired states
    this->shouldExistOnGpu = component->getExistsOnGpu();
    this->shouldEnableCpuMipmaps = component->getChunkData().getHasMipmaps();
    this->requestedActiveLod = component->getChunkManagerData().requestedActiveLod;
    this->requestedMaxLod = component->getChunkManagerData().requestedMaxLod;
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
                shouldCompletelyRegenerateMipmaps = true;
                lodRegenerationStartIndex = 0;

                break;
            }
            case SetOccupancy:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetOccupancy");

                auto command = commandBuffer->setOccupancyCommands.at(entry.index);
                chunkData.setVoxelOccupancy(command.position, command.isOccupied);

                shouldCompletelyWriteToGpu = true;

                // TODO: Incrementally update mipmaps
                // TODO: Incrementally update LODs

                break;
            }
            case SetMaterial:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetMaterial");

                auto command = commandBuffer->setMaterialCommands.at(entry.index);
                chunkData.setVoxelMaterialIndex(command.position, command.materialIndex);

                shouldCompletelyWriteToGpu = true;

                // TODO: Incrementally update LODs

                break;
            }
            case ClearOccupancy:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - ClearOccupancy");

                chunkData.clearOccupancyMap();

                shouldCompletelyWriteToGpu = true;
                shouldCompletelyRegenerateMipmaps = true;
                lodRegenerationStartIndex = 0;

                break;
            }
            case ClearMaterial:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - ClearMaterial");

                chunkData.clearMaterialMap();

                shouldCompletelyWriteToGpu = true;
                lodRegenerationStartIndex = 0;

                break;
            }
            case Copy:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - Copy");

                auto command = commandBuffer->copyCommands.at(entry.index);
                chunkData.copyFrom(*command.source);

                shouldCompletelyWriteToGpu = true;
                shouldCompletelyRegenerateMipmaps = true;
                lodRegenerationStartIndex = 0;

                break;
            }
            case SetExistsOnGpu:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetExistsOnGpu");

                // This command is deferred until the very end of the command buffer execution
                const auto command = commandBuffer->setExistsOnGpuCommands.at(entry.index);
                shouldExistOnGpu = command.existsOnGpu;

                shouldCompletelyWriteToGpu = true;

                break;
            }
            case SetEnableCpuMipmaps:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetEnableCpuMipmaps");

                // This command is deferred until the very end of the command buffer execution
                const auto command = commandBuffer->setEnableCpuMipmapsCommands.at(entry.index);
                shouldEnableCpuMipmaps = command.enableCpuMipmaps;

                break;
            }
            case SetActiveLod:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetActiveLod");

                // This command is deferred until the very end of the command buffer execution
                const auto command = commandBuffer->setActiveLodCommands.at(entry.index);
                Assert::isTrue(requestedMaxLod >= command.activeLod, "Requested LOD has not been generated");

                requestedActiveLod = command.activeLod;

                break;
            }
            case SetMaxLod:
            {
                ZoneScopedN("VoxelChunkCommandBuffer::apply - SetMaxLod");

                // This command is deferred until the very end of the command buffer execution
                const auto command = commandBuffer->setMaxLodCommands.at(entry.index);
                if (command.trim)
                {
                    requestedMaxLod = command.maxLod;
                }
                else
                {
                    requestedMaxLod = glm::max(requestedMaxLod, command.maxLod);
                }

                break;
            }
        }
    }

    updateMaxLod();
    updateActiveLod();
    updateGpu();
    updateMipmaps();
}

void VoxelChunkCommandBuffer::CommandApplicator::updateMipmaps()
{
    ZoneScoped;

    if (!component->getIsPartOfWorld())
    {
        Log::warning("Failed to apply VoxelChunkCommandBuffer. VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

        return;
    }

    auto& chunkData = component->getRawChunkData();

    if (shouldEnableCpuMipmaps != component->getRawChunkData().getHasMipmaps())
    {
        shouldCompletelyRegenerateMipmaps = true;
    }

    if (!shouldCompletelyRegenerateMipmaps)
    {
        return;
    }

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

        chunkData.setHasMipmaps(shouldEnableCpuMipmaps);
    }
    componentLock.lock();
}

void VoxelChunkCommandBuffer::CommandApplicator::updateActiveLod()
{
    ZoneScoped;

    if (!component->getIsPartOfWorld())
    {
        Log::warning("Failed to apply VoxelChunkCommandBuffer. VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

        return;
    }

    auto& chunkManagerData = component->getChunkManagerData();

    const auto previousActiveLod = chunkManagerData.activeLod;
    const auto activeLod = glm::min(requestedActiveLod, static_cast<int>(chunkManagerData.lods.size()));

    chunkManagerData.requestedActiveLod = requestedActiveLod;
    chunkManagerData.activeLod = activeLod;

    if (previousActiveLod != activeLod)
    {
        shouldCompletelyWriteToGpu = true;
    }
}

void VoxelChunkCommandBuffer::CommandApplicator::updateMaxLod()
{
    ZoneScoped;

    if (!component->getIsPartOfWorld())
    {
        Log::warning("Failed to apply VoxelChunkCommandBuffer. VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

        return;
    }

    auto& chunkManagerData = component->getChunkManagerData();

    // Calculate max possible LOD
    const int minSideLength = glm::min(glm::min(component->chunkData.getSize().x, component->chunkData.getSize().y), component->chunkData.getSize().z);
    const int maxPossibleLod = minSideLength == 0 ? 0 : glm::log2(minSideLength);

    // Update max LODs
    chunkManagerData.requestedMaxLod = requestedMaxLod;
    const int maxLodLevels = glm::min(requestedMaxLod, maxPossibleLod);
    auto& lods = chunkManagerData.lods;

    // Trim LOD count if needed
    while (lods.size() > maxLodLevels)
    {
        lods.pop_back();
    }

    // Add new LODs as needed
    while (lods.size() < maxLodLevels)
    {
        lods.emplace_back(std::make_shared<VoxelChunkData>());
    }

    // Regenerate LODs as needed
    if (lodRegenerationStartIndex < lods.size())
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

            for (int i = lodRegenerationStartIndex; i < lods.size(); ++i)
            {
                auto& previousLod = i > 0 ? *lods.at(i - 1) : component->chunkData;
                auto newLod = lods.at(i);

                previousLod.copyToLod(*newLod);
            }

#if DEBUG
            for (int i = 0; i < lods.size(); ++i)
            {
                Assert::isTrue(lods.at(i)->getSize() != glm::ivec3(0), "LOD has 0 size");
            }
#endif
        }
        componentLock.lock();
    }

    lodRegenerationStartIndex = lods.size();
}

void VoxelChunkCommandBuffer::CommandApplicator::updateGpu()
{
    ZoneScoped;

    if (!component->getIsPartOfWorld())
    {
        Log::warning("Failed to apply VoxelChunkCommandBuffer. VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

        return;
    }

    const auto& chunkManagerData = component->getChunkManagerData();

    if (!shouldExistOnGpu && component->getExistsOnGpu())
    {
        // Deallocate and exit
        component->deallocateGpuData();

        shouldCompletelyWriteToGpu = true;

        return;
    }

    if (!shouldCompletelyWriteToGpu)
    {
        // GPU is already up to date, skip writing to the GPU
        return;
    }

    // Write to the GPU
    {
        // Find active LOD and upload it to the GPU
        // We don't generate the LOD here
        const auto activeLodIndex = glm::min(chunkManagerData.activeLod, static_cast<int>(chunkManagerData.lods.size()));
        const VoxelChunkData& lod = activeLodIndex == 0 ? component->chunkData : *chunkManagerData.lods.at(activeLodIndex - 1);
        const auto lodSize = lod.getSize();

        // Break if size is not 0
        if (lodSize.x == 0 || lodSize.y == 0 || lodSize.z == 0)
        {
            return;
        }

        // This is a pointer to a unique pointer because we either use the existing VoxelChunk or allocate a new one
        // We use the existing VoxelChunk if possible, but that's not ideal in all cases
        //
        // If we need to resize the VoxelChunk, the resized VoxelChunk will have uninitialized data until we are done writing to it
        // This uninitialized data will be used by the renderer and cause a visual flash where uninitialized chunk data is displayed
        //
        // We want to avoid this flash, so we allocate a separate VoxelChunk and write our data there
        // After writing the data, we replace the VoxelChunk that the renderer uses
        const std::unique_ptr<VoxelChunk>* gpuData;
        std::unique_ptr<VoxelChunk> localGpuData; // Don't use. Only used for initialization purposes in the if statement below.
        if (component->getExistsOnGpu() && component->getChunk()->getSize() == lodSize)
        {
            // Existing data exists and does not need a resize
            // We will use it directly
            gpuData = &component->getChunk();
        }
        else
        {
            // Cannot use existing data
            // We will allocate a new VoxelChunk
            localGpuData = std::make_unique<VoxelChunk>(lodSize, false);
            gpuData = &localGpuData;
        }

        // We only need shared access because we are modifying a GPU resource
        // Writing takes a while so this is an optimization to prevent acquiring exclusive access for a long time when we don't need it
        componentLock.unlock();
        {
            std::shared_lock sharedComponentLock(component->getMutex());
            if (!component->getIsPartOfWorld())
            {
                Log::warning("Failed to apply VoxelChunkCommandBuffer. VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

                return;
            }

            // Check if unique pointer is valid
            // The dereference is because gpuData is a pointer to a unique pointer
            if (!*gpuData)
            {
                // Note that this case cannot happen when the VoxelChunkComponent
                // is only modified by the chunk modification threads.
                //
                // This is because the chunk modification threads respect command buffer submission order.
                Log::error("Attempted to write to an invalid unique pointer. This is unexpected and is probably a synchronization error.");

                return;
            }

            // Upload
            {
                std::lock_guard lockGpuUpload(gpuUploadLock);
                lod.copyTo(**gpuData);
            }
        }
        componentLock.lock();
        if (!component->getIsPartOfWorld())
        {
            Log::warning("Failed to apply VoxelChunkCommandBuffer. VoxelChunkComponent is no longer part of the world. This warning usually can be ignored.");

            return;
        }

        // Replace the VoxelChunk used by the component if needed
        if (localGpuData)
        {
            component->chunk = std::move(localGpuData);
            component->getTransform()->setLocalScale(glm::vec3(glm::pow(2, component->getChunkManagerData().activeLod)));
        }
    }
}
