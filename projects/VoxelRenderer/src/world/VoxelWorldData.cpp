#include "VoxelWorldData.h"

#include <src/world/VoxelWorldUtility.h>
#include <stdexcept>

void VoxelWorldData::copyFrom(VoxelWorld& world)
{
    size = world.getSize();
    occupancyMapIndices = world.getOccupancyMapIndices();

    data.resize(occupancyMapIndices.at(1));

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, world.getOccupancyMap().bufferId);
    glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, occupancyMapIndices.at(1), data.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void VoxelWorldData::writeTo(VoxelWorld& world)
{
    if (world.getSize() != size)
    {
        throw std::runtime_error("Target world does not have the same size");
    }

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, world.getOccupancyMap().bufferId);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, occupancyMapIndices.at(1), data.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    world.updateMipMaps();
}

void VoxelWorldData::clearOccupancy()
{
    std::fill(data.begin(), data.end(), 0);
}

const glm::ivec3& VoxelWorldData::getSize()
{
    return size;
}

void VoxelWorldData::setSize(glm::ivec3 size)
{
    this->size = size;
    occupancyMapIndices = VoxelWorldUtility::getOccupancyMapIndices(size);

    data.resize(occupancyMapIndices.at(1));
}
