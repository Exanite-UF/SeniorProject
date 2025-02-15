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
    // Please discuss with William the best practices for local storage of shader programs. Just to make sure you understand the nuance of when to store them.
    GLuint makeNoiseComputeProgram;
    GLuint makeMipMapComputeProgram;
    GLuint assignMaterialComputeProgram;

    glm::ivec3 size; // Size of the voxel world in voxels

    ShaderByteBuffer occupancyMap;
    int mipMapTextureCount;
    std::array<GLuint, 10> mipMapStartIndices; // There can only be 10 mip map textures. This gives 20 mip map levels

    ShaderByteBuffer materialMap; // This store the material data
    std::array<GLuint, 3> materialStartIndices; // There are 3 levels of the material data (This means the minimum size of a voxel world is 32 across)

    static constexpr glm::ivec3 minSize = { 32, 32, 32 };

    // GLuint occupancyMap; // This texture stores the raw voxel occupancy map, its first mipmap, and the first 3 bits of material data for each voxel
    // GLuint mipMap1; // This texture stores the second and third mip maps, and the second 3 bits of material data for each voxel
    // GLuint mipMap2; // This texture stores the fourth and fifth mip maps, and the final 3 bits of material data for each voxel
    // GLuint mipMap3; // This texture stores the sixth and seventh mip maps
    // GLuint mipMap4; // This texture stores the eighth and ninth mip maps

    double currentNoiseTime; // This variable is used to determine the "seed" used by the random functions in the make noise shader

    // TODO: Consider storing the width, height, and depth of the voxel world in this class
    // Remember that the voxel world dimensions are twice the size of the dimensions of the textures

    // TODO: Consider making these static functions, since they do not use the internal state of the class
    void makeNoise(ShaderByteBuffer& occupancyMap, double noiseTime, bool isRand2, float fillAmount); // This runs the make noise shader
    void makeMipMaps(ShaderByteBuffer& occupancyMap); // This runs the make mip map shader
    void assignMaterial(ShaderByteBuffer& materialMap, int level); // This runs the assign material shader

    void setSize(glm::ivec3 size);

public:
    Transform transform;

    // TODO: Consider having the width, height, and depth assigned by the constructor rather than hard coded.
    VoxelWorld(GLuint makeNoiseComputeProgram, GLuint makeMipMapComputeProgram, GLuint assignMaterialComputeProgram); // Creates a voxel world

    // isRand2 = Noise type toggle
    void generateFromNoise(double deltaTime, bool isRand2, float fillAmount);

    // TODO: Consider renaming
    // generateFromNoise also needs to bind textures. So calling this and then generateFromNoise will result in some of the textures that this functions binds being unbound
    void bindBuffers(int occupancyMap = 0, int materialMap = 1);
    void unbindBuffers() const;

    [[nodiscard]] glm::ivec3 getSize() const;

    [[nodiscard]] int getMipMapTextureCount() const;
    [[nodiscard]] std::array<GLuint, 10> getMipMapStartIndices() const;
    [[nodiscard]] std::array<GLuint, 3> getMaterialStartIndices() const;
};
