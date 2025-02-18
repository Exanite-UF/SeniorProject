#pragma once

#include <src/world/Camera.h>
#include <src/world/VoxelWorld.h>
#include <vector>
#include "src/graphics/GraphicsBuffer.h"

// The voxel renderer needs to be able to render multiple voxel worlds
class VoxelRenderer
{
private:
    // Odd Quirks of Images
    // RGB is not a valid format for an image. They must be either R, RG, or RGBA
    // Yes, openGL supports RGB textures, but in glsl there is no format specifier for rgb formated images (with a few odd exceptions)
    // As such any image buffer that needs 3 components is forced to use 4 components

    // These are 3D images
    // The x and y axes correspond to individual pixels
    // The z axis corresponds to different rays for the same pixel

    // These buffers are used to determine the starting configuration for a ray trace step
    // GLuint rayStartBuffer; //(x, y, z, isPerformingTrace)
    // GLuint rayDirectionBuffer; //(x, y, z, [unused])

    GraphicsBuffer<glm::vec3> rayStartBuffer;
    GraphicsBuffer<glm::vec3> rayDirectionBuffer;

    // These buffers are used to store the result of a ray trace step
    GraphicsBuffer<glm::vec4> rayHitPositionBuffer;//(x, y, z, wasHit)
    GraphicsBuffer<glm::vec4> rayHitNormalBuffer;//(x, y, z, depth)
    GraphicsBuffer<uint32_t> rayHitMaterialBuffer;//(material)
    GraphicsBuffer<glm::vec3> rayHitVoxelPositionBuffer;//(x, y, z)

    GraphicsBuffer<glm::vec3> attentuationBuffer;//(r, g, b)
    GraphicsBuffer<glm::vec3> accumulatedLightBuffer;//(r, g, b)

    GLuint materialTexturesBuffer;//This buffer will store the structs of material textures



    // These are compute shaders that are used to render
    static GLuint prepareRayTraceFromCameraProgram;
    static GLuint executeRayTraceProgram;
    static GLuint resetHitInfoProgram;
    static GLuint displayToWindowProgram;
    static GLuint BRDFProgram;

    int xSize = 0;
    int ySize = 0;
    int raysPerPixel = 0;

    bool isSizingDirty = true; // This is used to automatically remake the buffers only if the size of the buffers has changed

    // This makes the images using the size and rays per pixel
    // It remakes them if textures are already bound
    void remakeTextures();

    void handleDirtySizing(); // Remakes the textures if the sizing is dirty

public:
    VoxelRenderer();

    void setResolution(int x, int y);
    void setRaysPerPixel(int number);

    void prepareRayTraceFromCamera(const Camera& camera);

    void executeRayTrace(std::vector<VoxelWorld>& worlds);

    //materialMap: This maps from material index to material id (What is stored in the material result to an actual material)
    //materialTextureSizes: This stores the size of voxel in each material texture (it is 512 vec2 values)
    void accumulateLight(const std::array<uint32_t, 4096>& materialMap, const std::array</*Some struct*/, 512>& materialTextures);//Some struct must be 48 byte large

    void display();
};
