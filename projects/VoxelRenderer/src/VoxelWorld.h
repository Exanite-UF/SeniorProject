#pragma once

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>

class VoxelWorld
{
private:
    // Please discuss with William the best practices for local storage of shader programs. Just to make sure you understand the nuance of when to store them.
    GLuint makeNoiseComputeProgram;
    GLuint makeMipMapComputeProgram;
    GLuint assignMaterialComputeProgram;

    GLuint occupancyMap; // This texture stores the raw voxel occupancy map, its first mipmap, and the first 3 bits of material data for each voxel
    GLuint mipMap1; // This texture stores the second and third mip maps, and the second 3 bits of material data for each voxel
    GLuint mipMap2; // This texture stores the fourth and fifth mip maps, and the final 3 bits of material data for each voxel
    GLuint mipMap3; // This texture stores the sixth and seventh mip maps
    GLuint mipMap4; // This texture stores the eighth and ninth mip maps

    double currentNoiseTime; // This variable is used to determine the "seed" used by the random functions in the make noise shader

    glm::vec3 position = glm::vec3(0, 0, 0);
    glm::vec3 scale = glm::vec3(1, 1, 1);
    glm::quat orientation = glm::quat(1, 0, 0, 0);

    // TODO: Consider storing the width, height, and depth of the voxel world in this class
    // Remember that the voxel world dimensions are twice the size of the dimensions of the textures

    // TODO: Consider making these static functions, since they do not use the internal state of the class
    void makeNoise(GLuint image3D, double noiseTime, bool isRand2, float fillAmount); // This runs the make noise shader
    void makeMipMap(GLuint inputImage3D, GLuint outputImage3D); // This runs the make mip map shader
    void assignMaterial(GLuint image3D); // This runs the assign material shader

    glm::ivec3 getTextureSize() const; // TODO: implement

public:
    // TODO: Consider having the width, height, and depth assigned by the constructor rather than hard coded.
    VoxelWorld(GLuint makeNoiseComputeProgram, GLuint makeMipMapComputeProgram, GLuint assignMaterialComputeProgram); // Creates a voxel world

    // isRand2 = Noise type toggle
    void generateFromNoise(double deltaTime, bool isRand2, float fillAmount);

    // TODO: Consider renaming
    // generateFromNoise also needs to bind textures. So calling this and then generateFromNoise will result in some of the textures that this functions binds being unbound
    void bindTextures() const;
    void unbindTextures() const;

    glm::ivec3 getSize() const; // TODO: implement

    glm::vec3 getPosition() const;
    glm::vec3 getScale() const;
    glm::quat getOrientation() const;
};
