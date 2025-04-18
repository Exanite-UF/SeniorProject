#pragma once

#include <src/world/VoxelChunkComponent.h>

#include "SceneComponent.h"

// A command buffer for commands related to modifying a VoxelChunkComponent
class VoxelChunkCommandBuffer
{
    friend class VoxelChunkManager;

private:
    enum CommandType
    {
        SetSize,
        SetOccupancy,
        SetMaterial,
        ClearOccupancy,
        ClearMaterial,
        Copy,
        SetExistsOnGpu,
        SetEnableCpuMipmaps,
        SetActiveLod,
        SetMaxLod,
    };

    struct Command
    {
        CommandType type;
        int32_t index;

        explicit Command(const CommandType type, const int32_t index)
        {
            this->type = type;
            this->index = index;
        }
    };

    struct SetSizeCommand
    {
        glm::ivec3 size;

        explicit SetSizeCommand(const glm::ivec3& size)
        {
            this->size = size;
        }
    };

    struct SetOccupancyCommand
    {
        glm::ivec3 position;
        bool isOccupied;

        explicit SetOccupancyCommand(const glm::ivec3& position, const bool isOccupied)
        {
            this->position = position;
            this->isOccupied = isOccupied;
        }
    };

    struct SetMaterialCommand
    {
        glm::ivec3 position;
        uint16_t materialIndex;

        explicit SetMaterialCommand(const glm::ivec3& position, const uint16_t materialIndex)
        {
            this->position = position;
            this->materialIndex = materialIndex;
        }
    };

    struct CopyCommand
    {
        std::shared_ptr<VoxelChunkData> source;

        explicit CopyCommand(const std::shared_ptr<VoxelChunkData>& source)
        {
            this->source = source;
        }
    };

    struct SetExistsOnGpuCommand
    {
        bool existsOnGpu;

        explicit SetExistsOnGpuCommand(const bool existsOnGpu)
        {
            this->existsOnGpu = existsOnGpu;
        }
    };

    struct SetEnableCpuMipmapsCommand
    {
        bool enableCpuMipmaps;

        explicit SetEnableCpuMipmapsCommand(const bool enableCpuMipmaps)
        {
            this->enableCpuMipmaps = enableCpuMipmaps;
        }
    };

    struct SetActiveLodCommand
    {
        int activeLod;

        explicit SetActiveLodCommand(const int activeLod)
        {
            this->activeLod = activeLod;
        }
    };

    struct SetMaxLodCommand
    {
        int maxLod;
        bool trim;

        explicit SetMaxLodCommand(const int maxLod, const bool trim)
        {
            this->maxLod = maxLod;
            this->trim = trim;
        }
    };

    std::vector<Command> commands {};

    std::vector<SetSizeCommand> setSizeCommands {};
    std::vector<SetOccupancyCommand> setOccupancyCommands {};
    std::vector<SetMaterialCommand> setMaterialCommands {};
    std::vector<CopyCommand> copyCommands {};
    std::vector<SetExistsOnGpuCommand> setExistsOnGpuCommands {};
    std::vector<SetEnableCpuMipmapsCommand> setEnableCpuMipmapsCommands {};
    std::vector<SetActiveLodCommand> setActiveLodCommands {};
    std::vector<SetMaxLodCommand> setMaxLodCommands {};

public:
    void setSize(const glm::ivec3& size);

    void setVoxelOccupancy(const glm::ivec3& position, bool isOccupied);
    void setVoxelMaterial(const glm::ivec3& position, const std::shared_ptr<Material>& material);
    void setVoxelMaterialIndex(const glm::ivec3& position, uint16_t materialIndex);

    void clearOccupancyMap();
    void clearMaterialMap();

    void copyFrom(const std::shared_ptr<VoxelChunkData>& data);

    void setExistsOnGpu(bool existsOnGpu);

    void setEnableCpuMipmaps(bool enableCpuMipmaps);

    void setActiveLod(int activeLod);
    void setMaxLod(int maxLod, bool trim = false);

    void clear();

    // This will copy the commands of this command buffer into the other command buffer.
    // The commands of this buffer will be placed AFTER the commands of the other buffer.
    void mergeInto(VoxelChunkCommandBuffer& other) const;

private:
    class CommandApplicator
    {
        // ----- Inputs -----

        const VoxelChunkCommandBuffer* commandBuffer;
        std::shared_ptr<VoxelChunkComponent> component;
        std::shared_ptr<SceneComponent> scene;

        CancellationToken cancellationToken {};

        // ----- Synchronization -----

        std::unique_lock<std::shared_mutex> componentLock;
        std::unique_lock<std::mutex> gpuUploadLock;

        // ----- Change tracking -----

        // TODO: Track exact changes for optimized CPU -> GPU copies
        // TODO: Note that change tracking also needs to consider the active LOD

        bool shouldCompletelyWriteToGpu = false;
        bool shouldCompletelyRegenerateMipmaps = false;

        // If this is equal or greater than the actual max LOD count, then no regeneration needs to be done
        // Set to 0 to regenerate all LODs
        int lodRegenerationStartIndex = 0;

        // ----- Deferred desired states -----

        bool shouldExistOnGpu = false;
        bool shouldEnableCpuMipmaps = false;

    public:
        explicit CommandApplicator(const VoxelChunkCommandBuffer* commandBuffer, const std::shared_ptr<VoxelChunkComponent>& component, const std::shared_ptr<SceneComponent>& scene, std::mutex& gpuUploadMutex, const CancellationToken& cancellationToken);

        void apply();

    private:
        void updateMipmaps();
        void updateMaxLod();
        void updateGpu();
    };

    // Warning: apply() will acquire the relevant mutexes. Do not acquire them yourself.
    void apply(const std::shared_ptr<VoxelChunkComponent>& component, const std::shared_ptr<SceneComponent>& scene, std::mutex& gpuUploadMutex, const CancellationToken& cancellationToken) const;
};
