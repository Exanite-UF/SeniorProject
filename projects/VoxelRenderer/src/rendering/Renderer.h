#pragma once
#include "AsynchronousReprojection.h"
#include "PostProcessing.h"
#include "VoxelRenderer.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

#include <src/world/Camera.h>
#include <src/world/Scene.h>

class VoxelRenderer;
class AsynchronousReprojection;

class Renderer
{
private:
    // Profiling
    int renderCount = 0;
    int reprojectionCount = 0;

    float overdrawFOV = 0;
    constexpr static float maxFov = 3.1415926589 * 0.9;

    glm::vec2 upscaleMultiplier = { 1, 1 };

private:
    // Rendering Contexts
    GLFWwindow* offscreenContext = nullptr;
    GLFWwindow* mainContext = nullptr;

    std::thread offscreenThread;
    bool isRenderingOffscreen = false;
    bool isSizeDirtyThread = false;

    void offscreenRenderingFunc();

private:
    // Camera stuff
    glm::vec3 olderRenderedPosition;

    std::mutex cameraMtx;
    glm::vec3 lastRenderedPosition;
    glm::quat lastRenderedRotation;
    float lastRenderedFOV;

    glm::vec3 currentCameraPosition;
    glm::quat currentCameraRotation;
    float currentCameraFOV;

private:
    // Rendering variables

    glm::ivec2 renderResolution;

    std::thread::id owningThread; // The id of the thread that owns the asynchronous reprojection framebuffer objects (The thread that needs to render to the framebuffers)
    std::array<GLuint, 3> framebuffers { 0 }; // These are the three framebuffer objects that are used as input to asynchronous reprojection
    std::array<GLenum, 4> drawBuffers = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 }; // These are the buffers that the shader will write to when rendering to one of the three framebuffers (Use glDrawBuffers with this)

    struct BufferMapping
    {
        std::uint8_t display = 0;
        std::uint8_t ready = 1;
        std::uint8_t working = 2;
    } bufferMapping;

    bool isNewerFrame = false;

    struct BufferLocks
    {
        std::recursive_mutex display;
        std::recursive_mutex ready;
        std::recursive_mutex working;
    } bufferLocks;

    std::array<GLuint, 3> colorTextures;
    std::array<GLuint, 3> positionTextures;
    std::array<GLuint, 3> normalTextures;
    std::array<GLuint, 3> materialTextures;

private:
    Scene* scene = nullptr;
    int bounces = 2;

    std::unique_ptr<VoxelRenderer> voxelRenderer = nullptr;
    std::unique_ptr<AsynchronousReprojection> reprojection = nullptr;
    std::unique_ptr<PostProcessing> postProcessing = nullptr;

    static GLuint drawTextureProgram;

    friend class VoxelRenderer;
    friend class AsynchronousReprojection;

    // This is what the reprojection and post processes work on
    std::recursive_mutex outputLock;
    glm::ivec2 outputResolution;
    GLuint outputDepthTexture;
    GLuint outputColorTexture;
    GLuint outputPositionTexture;
    GLuint outputNormalTexture;
    GLuint outputMaterialTexture;

    // Asserts that the calling thread is the owning thread of the framebuffers
    // Will crash on failure
    void isOwningThreadCheck();

    void _render(); // This is where all the rendering happens (The underscore is because a publicly facing function that wraps the entire rendering process exists)
    void reproject(float fov = -1); // This is where reprojection occurs
    void postProcess(); // This is where post processing happens
    void finalDisplay(); // Actually displays the image to the screen. Must run after post processing

    void makeOutputTextures();

public:
    Renderer(GLFWwindow* mainContext, GLFWwindow* offscreenContext);

    // This needs to be called on the thread that needs to render to the asynchronous reprojection input
    void makeFramebuffers();

    // Grabs the most recently rendered frame.
    void swapDisplayBuffer();

    // Pushes out the most recently rendered frame.
    // Also calls AsynchronousReprojection to combine the frames
    void swapWorkingBuffer();

    // Gets the framebuffer that is rendered to asynchronously
    // Will crash if the wrong thread calls this function
    // Only the thread that is rendering asynchronously may call this function
    GLuint getWorkingFramebuffer();

    void setRenderResolution(glm::ivec2 renderResolution);
    void setRaysPerPixel(int number);

    void pollCamera(const Camera& camera);
    void setScene(Scene& scene);
    void setBounces(const int& bounces);

    void render(float fov = -1);

    void startAsynchronousReprojection();
    void stopAsynchronousReprojection();
    void toggleAsynchronousReprojection();

    int getRenderCounter();
    void resetRenderCounter();

    int getReprojectionCounter();
    void resetReprojectionCounter();

    void setFPSLimit(float fps);
    void disableFPSLimit();

    void setAsynchronousOverdrawFOV(float extraFOV);

    std::shared_ptr<PostProcess> addPostProcessEffect(std::shared_ptr<PostProcess> effect);

    glm::vec2 getUpscaleMultiplier();

    glm::vec3 getCurrentCameraPosition();
    glm::quat getCurrentCameraRotation();
    float getCurrentCameraFOV();

    glm::ivec2 getUpscaleResolution();
};
