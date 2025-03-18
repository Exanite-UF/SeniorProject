#pragma once

#include <mutex>
#include <semaphore>
#include <thread>
#include <vector>

#include <src/graphics/GraphicsBuffer.h>
#include <src/rendering/AsynchronousReprojection.h>
#include <src/rendering/Renderer.h>
#include <src/utilities/NonCopyable.h>
#include <src/world/CameraComponent.h>
#include <src/world/MaterialManager.h>
#include <src/world/VoxelChunkComponent.h>

class Renderer;

// The voxel renderer needs to be able to render multiple voxel worlds
class VoxelRenderer : public NonCopyable
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

    // These are used as input and output
    GraphicsBuffer<glm::vec3> rayStartBuffer1;
    GraphicsBuffer<glm::vec3> rayDirectionBuffer1;
    GraphicsBuffer<glm::vec3> rayStartBuffer2;
    GraphicsBuffer<glm::vec3> rayDirectionBuffer2;

    // These buffers are used to store the result of a ray trace step
    GraphicsBuffer<float> rayHitMiscBuffer; //(wasHit, depth)

    GraphicsBuffer<glm::vec3> attentuationBuffer1; //(r, g, b)
    GraphicsBuffer<glm::vec3> accumulatedLightBuffer1; //(r, g, b)
    GraphicsBuffer<glm::vec3> attentuationBuffer2; //(r, g, b)
    GraphicsBuffer<glm::vec3> accumulatedLightBuffer2; //(r, g, b)

    // Used as the final output buffers
    GraphicsBuffer<glm::vec3> normalBuffer;
    GraphicsBuffer<glm::vec3> positionBuffer;
    GraphicsBuffer<glm::vec3> materialBuffer; //(roughness, _, _)

    int currentBuffer = 0;

    GLuint materialTexturesBuffer; // This buffer will store the structs of material textures

    // These are compute shaders that are used to render
    static GLuint prepareRayTraceFromCameraProgram;
    static GLuint resetHitInfoProgram;
    static GLuint afterCastProgram;
    static GLuint resetVisualInfoProgram;
    static GLuint fullCastProgram;
    static GLuint pathTraceToFramebufferProgram;

    glm::ivec2 size {};
    int raysPerPixel = 0;

    bool isSizingDirty = true; // This is used to automatically remake the buffers only if the size of the buffers has changed

    // This makes the images using the size and rays per pixel
    // It remakes them if textures are already bound
    void remakeTextures();

    void handleDirtySizing(); // Remakes the textures if the sizing is dirty

    VoxelRenderer(); // This is only supposed to be ran by Renderer

    friend class Renderer;

    void afterCast();

public:
    void setResolution(glm::ivec2 size);
    void setRaysPerPixel(int number);

    void prepareRayTraceFromCamera(const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV, bool resetLight = true);

    void resetHitInfo();

    void resetVisualInfo(bool resetLight = true, bool resetAttenuation = true, bool resetFirstHit = true, bool drawSkyBox = true);

    void executeRayTrace(std::vector<std::shared_ptr<VoxelChunkComponent>>& chunks, bool isFirstRay);

    void executePathTrace(std::vector<std::shared_ptr<VoxelChunkComponent>>& chunks, int bounces);

    void render(const GLuint& framebuffer, const std::array<GLenum, 4>& drawBuffers, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV);
};
