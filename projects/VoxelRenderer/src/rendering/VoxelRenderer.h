#pragma once

#include <mutex>
#include <semaphore>
#include <thread>
#include <unordered_map>
#include <vector>

#include <src/graphics/GraphicsBuffer.h>
#include <src/graphics/Texture.h>
#include <src/rendering/AsynchronousReprojection.h>
#include <src/rendering/Renderer.h>
#include <src/utilities/NonCopyable.h>
#include <src/world/CameraComponent.h>
#include <src/world/MaterialManager.h>
#include <src/world/SceneComponent.h>
#include <src/world/SkyboxComponent.h>
#include <src/world/VoxelChunkComponent.h>

#include <glm/glm.hpp>
#include <glm/gtc/packing.hpp> // For packing/unpacking
#include <glm/gtc/type_precision.hpp> // Provides uint16

class Renderer;

// The voxel renderer needs to be able to render multiple voxel chunks
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

    // A ray trace step needs to exist that goes through a single chunk and gets the hit info

    // These are used as input and output
    bool whichStartBuffer = false;
    GraphicsBuffer<glm::vec3> rayStartBuffer1; // This is where rays will start from
    GraphicsBuffer<glm::vec3> rayDirectionBuffer1; // This is the direction rays will go in //convert to half
    GraphicsBuffer<glm::vec3> rayStartBuffer2; // This is where rays will start from
    GraphicsBuffer<glm::vec3> rayDirectionBuffer2; // This is the direction rays will go in //convert to half

    // These buffers are used to store the result of a path trace
    bool whichAccumulationBuffer = false;
    GraphicsBuffer<glm::vec3> attentuationBuffer1; //(r, g, b) //convert to half
    GraphicsBuffer<glm::vec3> accumulatedLightBuffer1; //(r, g, b) //convert to half
    GraphicsBuffer<glm::vec3> attentuationBuffer2; //(r, g, b) //convert to half
    GraphicsBuffer<glm::vec3> accumulatedLightBuffer2; //(r, g, b) //convert to half

    // This is reset before every cast
    GraphicsBuffer<float> rayMisc; //(depth)

    // These are primary ray info
    bool whichDepth = false;
    GraphicsBuffer<glm::vec4> normalBuffer; // (x, y, z, depth 1) world space
    GraphicsBuffer<glm::vec4> positionBuffer; // (x, y, z, depth 2) world space
    GraphicsBuffer<glm::u16vec2> materialUVBuffer; //(u, v)
    GraphicsBuffer<std::int32_t> materialBuffer; //(materialID)
    GraphicsBuffer<glm::vec3> primaryDirection; //(x, y, z) //convert to half
    GraphicsBuffer<glm::vec4> secondaryDirection; //(x, y, z, w) w is the scaling needed from the pdf of sampling distribution //convert to half

    bool whichSampleRadiance = false;
    GraphicsBuffer<glm::u16vec4> sampleDirection1; //(x, y, z, w) w is the scaling needed from the pdf of sampling distribution
    GraphicsBuffer<glm::u16vec4> sampleDirection2; //(x, y, z, w) w is the scaling needed from the pdf of sampling distribution
    GraphicsBuffer<glm::u16vec3> sampleRadiance1; //(r, g, b)
    GraphicsBuffer<glm::u16vec3> sampleRadiance2; //(r, g, b)
    GraphicsBuffer<glm::u16vec3> sampleWeights1;
    GraphicsBuffer<glm::u16vec3> sampleWeights2;

    bool whichMotionVectors = false;
    GraphicsBuffer<glm::u16vec4> motionVectors; // This accumulates motion vector values to prevent undershooting from sub pixel movement

    GLuint materialTexturesBuffer; // This buffer will store the structs of material textures

    // These are compute shaders that are used to render
    static GLuint prepareRayTraceFromCameraProgram;
    static GLuint resetHitInfoProgram;
    static GLuint afterCastProgram;
    static GLuint resetVisualInfoProgram;
    static GLuint fullCastProgram;
    static GLuint pathTraceToFramebufferProgram;

    static GLuint resetPrimaryRayInfoProgram;
    static GLuint beforeCastProgram;
    static GLuint primaryRayProgram;
    static GLuint groupPixelsProgram;

    glm::ivec2 size {};

    bool isSizingDirty = true; // This is used to automatically remake the buffers only if the size of the buffers has changed

    // This makes the images using the size and rays per pixel
    // It remakes them if textures are already bound
    void remakeTextures();

    void handleDirtySizing(); // Remakes the textures if the sizing is dirty

    VoxelRenderer(); // This is only supposed to be ran by Renderer

    friend class Renderer;

    float maxDepth = 10000.0;

public:
    void setResolution(glm::ivec2 size);

    // This sets the ray directions
    // It also resets all data that determined by a path trace
    void prepareRayTraceFromCamera(const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV, bool resetLight = true);

    // Resets information that is determined by primary rays
    void resetPrimaryRayInfo();

    void resetVisualInfo(float maxDepth);
    void beforeCast(float maxDepth, std::shared_ptr<SceneComponent> scene, bool shouldDrawSkybox = true);
    void afterCast(float maxDepth);

    void executePrimaryRay(const std::vector<std::shared_ptr<VoxelChunkComponent>>& chunks, const glm::vec3& pastCameraPosition, const glm::quat& pastCameraRotation, const float& pastCameraFOV, std::shared_ptr<SceneComponent> scene);

    void executeRayTrace(const std::vector<std::shared_ptr<VoxelChunkComponent>>& chunks, const glm::vec3& pastCameraPosition, const glm::quat& pastCameraRotation, const float& pastCameraFOV, int shadingRate, const glm::ivec2& offset, std::shared_ptr<SceneComponent> scene);

    void executePathTrace(const std::vector<std::shared_ptr<VoxelChunkComponent>>& chunks, int bounces, const glm::vec3& pastCameraPosition, const glm::quat& pastCameraRotation, const float& pastCameraFOV, std::shared_ptr<SceneComponent> scene);

    void render(const GLuint& framebuffer, const std::array<GLenum, 4>& drawBuffers, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV, std::shared_ptr<SceneComponent> scene);
};
