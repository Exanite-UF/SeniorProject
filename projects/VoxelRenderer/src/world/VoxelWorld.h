#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <array>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

#include <src/graphics/ShaderByteBuffer.h>
#include <src/graphics/ShaderFloatBuffer.h>
#include <src/world/Transform.h>

class VoxelWorld
{
private:
    static constexpr glm::ivec3 minSize = { 32, 32, 32 };

    glm::ivec3 size; // Size of the voxel world in voxels

    ShaderByteBuffer occupancyMap;
    int mipMapTextureCount;
    std::array<GLuint, 10> mipMapStartIndices; // There can only be 10 mip map textures. This gives 20 mip map levels

    ShaderByteBuffer materialMap; // This store the material data
    std::array<GLuint, 3> materialStartIndices; // There are 3 levels of the material data (This means the minimum size of a voxel world is 32 across)

    double currentNoiseTime = 0; // This variable is used to determine the "seed" used by the random functions in the make noise shader
    GLuint makeNoiseComputeProgram = 0; // TODO: Consider moving makeNoise to a world generator class

    void generateOccupancyUsingNoise(ShaderByteBuffer& occupancyMap, double noiseTime, bool isRand2, float fillAmount); // This runs the make noise shader

    GLuint makeMipMapComputeProgram = 0;
    void updateMipMaps(ShaderByteBuffer& occupancyMap); // This runs the make mip map shader

    GLuint assignMaterialComputeProgram = 0; // TODO: Consider moving assignMaterial to a world generator class
    void assignMaterial(ShaderByteBuffer& materialMap, int level); // This runs the assign material shader

    void setSize(glm::ivec3 size);

public:
    Transform transform;

    // TODO: Consider having the width, height, and depth assigned by the constructor rather than hard coded.
    // Creates a voxel world
    VoxelWorld(GLuint makeNoiseComputeProgram, GLuint makeMipMapComputeProgram, GLuint assignMaterialComputeProgram);

    // isRand2 = Noise type toggle
    // TODO: Yes, this is a really long name. No, I do not like it, but I'm not sure what else to call it.
    void generateOccupancyAndMipMapsAndMaterials(double deltaTime, bool isRand2, float fillAmount);

    // generateFromNoise also needs to bind textures. So calling this and then generateOccupancyAndMipMapsAndMaterials will result in some of the textures that this functions binds being unbound
    void bindBuffers(int occupancyMapIndex = 0, int materialMapIndex = 1);
    void unbindBuffers() const;

    [[nodiscard]] glm::ivec3 getSize() const;

    [[nodiscard]] int getMipMapTextureCount() const;
    [[nodiscard]] std::array<GLuint, 10> getMipMapStartIndices() const;
    [[nodiscard]] std::array<GLuint, 3> getMaterialStartIndices() const;
};
