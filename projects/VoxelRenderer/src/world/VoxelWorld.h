#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <src/graphics/GraphicsBuffer.h>
#include <src/utilities/NonCopyable.h>
#include <src/utilities/OpenGl.h>

class VoxelWorld : public NonCopyable
{
private:
    glm::ivec3 size; // Size of the voxel world in voxels

    GraphicsBuffer<uint8_t> occupancyMap; // This stores the voxel occupancy data
    std::vector<GLuint> occupancyMapIndices; // The start indices of the occupancy map in bytes. The max size of this vector is defined by Constants::VoxelWorld::maxOccupancyMapLayerCount

    GraphicsBuffer<uint16_t> materialMap; // This stores the voxel material data

public:
    explicit VoxelWorld(glm::ivec3 size);

    void setSize(glm::ivec3 size);
    [[nodiscard]] glm::ivec3 getSize() const;

    [[nodiscard]] const GraphicsBuffer<uint8_t>& getOccupancyMap();
    [[nodiscard]] std::vector<GLuint> getOccupancyMapIndices() const;

    [[nodiscard]] const GraphicsBuffer<uint16_t>& getMaterialMap();

public:
    void updateMipMaps();
    void generatePlaceholderData(double deltaTime, bool useRandomNoise, float fillAmount);

private:
    double currentNoiseTime = 0; // This variable is used to determine the "seed" used by the random functions in the make noise shader

    void generateNoiseOccupancyMap(double noiseTime, bool useRandomNoise, float fillAmount); // This runs the make noise shader
    void generatePlaceholderMaterialMap(); // This runs the assign material shader

public:
    // generateNoiseOccupancyMap also needs to bind textures. So calling this and then generateNoiseOccupancyMapAndMipMaps will result in some of the textures that this functions binds being unbound
    void bindBuffers(int occupancyMapIndex = 0, int materialMapIndex = 1);
    void unbindBuffers() const;
};
