#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <src/Constants.h>
#include <src/graphics/GraphicsBuffer.h>
#include <src/utilities/NonCopyable.h>

class VoxelWorld : public NonCopyable
{
private:
    glm::ivec3 size; // Size of the voxel world in voxels

    GraphicsBuffer<uint8_t> occupancyMap; // This stores the voxel occupancy data
    std::vector<GLuint> occupancyMapIndices; // The start indices of the occupancy map in bytes. The max size of this vector is defined by Constants::VoxelWorld::maxOccupancyMapLayerCount

    GraphicsBuffer<uint16_t> materialMap; // This stores the voxel material data

    double currentNoiseTime = 0; // This variable is used to determine the "seed" used by the random functions in the make noise shader
    GLuint makeNoiseComputeProgram = 0; // TODO: Consider moving makeNoise to a world generator class
    GLuint makeMipMapComputeProgram = 0;
    GLuint assignMaterialComputeProgram = 0; // TODO: Consider moving assignMaterial to a world generator class

    void generateNoiseOccupancyMap(double noiseTime, bool isRand2, float fillAmount); // This runs the make noise shader
    void generatePlaceholderMaterialMap(); // This runs the assign material shader

    void setSize(glm::ivec3 size);

public:
    // Creates a voxel world
    VoxelWorld(glm::ivec3 size, GLuint makeNoiseComputeProgram, GLuint makeMipMapComputeProgram, GLuint assignMaterialComputeProgram);

    [[nodiscard]] glm::ivec3 getSize() const;

    [[nodiscard]] const GraphicsBuffer<uint8_t>& getOccupancyMap();
    [[nodiscard]] std::vector<GLuint> getOccupancyMapIndices() const;

    [[nodiscard]] const GraphicsBuffer<uint16_t>& getMaterialMap();

    void updateMipMaps(); // This runs the make mip map shader

    // isRand2 = Noise type toggle
    // TODO: Yes, this is a really long name. No, I do not like it, but I'm not sure what else to call it.
    void generateNoiseOccupancyMapAndMipMaps(double deltaTime, bool isRand2, float fillAmount);

    // generateNoiseOccupancyMap also needs to bind textures. So calling this and then generateNoiseOccupancyMapAndMipMaps will result in some of the textures that this functions binds being unbound
    void bindBuffers(int occupancyMapIndex = 0, int materialMapIndex = 1);
    void unbindBuffers() const;
};
