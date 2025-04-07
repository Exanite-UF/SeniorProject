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
        bool writeToGpu;

        explicit SetExistsOnGpuCommand(const bool existsOnGpu, const bool writeToGpu)
        {
            this->existsOnGpu = existsOnGpu;
            this->writeToGpu = writeToGpu;
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

        explicit SetMaxLodCommand(const int maxLod)
        {
            this->maxLod = maxLod;
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

    void setExistsOnGpu(bool existsOnGpu, bool writeToGpu = true);

    void setEnableCpuMipmaps(bool enableCpuMipmaps);

    void setActiveLod(int activeLod);
    void setMaxLod(int maxLod);

    void clear();

private:
    // Warning: apply() will acquire the relevant mutexes. Do not acquire them yourself.
    void apply(const std::shared_ptr<VoxelChunkComponent>& component, const std::shared_ptr<SceneComponent>& scene, std::mutex& gpuUploadMutex) const;
};
