#pragma once

#include <src/world/VoxelWorld.h>

class VoxelWorldData
{
private:
    glm::ivec3 size;
    std::vector<GLuint> occupancyStartIndices;

    std::vector<uint8_t> data;

public:
    [[nodiscard]] const glm::ivec3& getSize();
    void setSize(glm::ivec3 size);

    void copyFrom(VoxelWorld& world);
    void writeTo(VoxelWorld& world);

    void clearOccupancy();
};
