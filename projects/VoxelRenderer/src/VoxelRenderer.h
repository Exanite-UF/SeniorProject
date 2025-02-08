#pragma once

#include "Camera.h"
#include "ShaderFloatBuffer.h"
#include "VoxelWorld.h"
#include <vector>

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

    ShaderFloatBuffer rayStartBuffer;
    ShaderFloatBuffer rayDirectionBuffer;

    // These buffers are used to store the result of a ray trace step
    GLuint rayHitPositionBuffer; //(x, y, z, wasHit)
    GLuint rayHitNormalBuffer; //(x, y, z, depth)
    GLuint rayHitMaterialBuffer; //(material)

    // These are compute shaders that are used to render
    static GLuint prepareRayTraceFromCameraProgram;
    static GLuint executeRayTraceProgram;
    static GLuint resetHitInfoProgram;
    static GLuint displayToWindowProgram;

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

    void display();
};
