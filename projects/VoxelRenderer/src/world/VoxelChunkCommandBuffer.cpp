#include "VoxelChunkCommandBuffer.h"

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

void VoxelChunkCommandBuffer::apply(const std::shared_ptr<VoxelChunkComponent>& component) const
{
    // TODO: Track exact changes for optimized CPU -> GPU copies
    auto& chunk = component->getChunk();
    auto& chunkData = component->getChunkData();

    for (const auto entry : commands)
    {
        switch (entry.type)
        {
            case SetSize:
            {
                auto command = setSizeCommands.at(entry.index);
                chunkData.setSize(command.size);

                break;
            }
            case SetOccupancy:
            {
                auto command = setOccupancyCommands.at(entry.index);
                chunkData.setVoxelOccupancy(command.position, command.isOccupied);

                break;
            }
            case SetMaterial:
            {
                auto command = setMaterialCommands.at(entry.index);
                chunkData.setVoxelMaterialIndex(command.position, command.materialIndex);

                break;
            }
            case ClearOccupancy:
            {
                chunkData.clearOccupancyMap();

                break;
            }
            case ClearMaterial:
            {
                chunkData.clearMaterialMap();

                break;
            }
            case Copy:
            {
                auto command = copyCommands.at(entry.index);
                chunkData.copyFrom(*command.source);

                break;
            }
        }
    }

    chunkData.writeTo(*chunk);
}

void VoxelChunkCommandBuffer::clear()
{
    setSizeCommands.clear();
    setOccupancyCommands.clear();
    setMaterialCommands.clear();
    copyCommands.clear();
}
