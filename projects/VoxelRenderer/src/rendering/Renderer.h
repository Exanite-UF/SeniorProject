#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <chrono>
#include <memory>
#include <mutex>
#include <thread>

#include <src/rendering/AsynchronousReprojection.h>
#include <src/rendering/PostProcessing.h>
#include <src/rendering/SVGF.h>
#include <src/rendering/VoxelRenderer.h>
#include <src/utilities/OpenGl.h>
#include <src/windowing/GlfwContext.h>
#include <src/windowing/Window.h>
#include <src/world/CameraComponent.h>
#include <src/world/SceneComponent.h>

class VoxelRenderer;
class AsyncReprojectionRenderer;
class SVGF;

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
    std::shared_ptr<Window> mainContext = nullptr;
    std::shared_ptr<GlfwContext> offscreenContext = nullptr;

    std::thread offscreenThread;
    bool _isRenderingOffscreen = false;
    bool isSizeDirtyThread = false;
    bool _isRenderingPaused = false;

    void offscreenRenderingFunc();

private:
    // Camera stuff
    glm::vec3 olderRenderedPosition {};

    std::mutex cameraMtx {};
    glm::vec3 lastRenderedPosition {};
    glm::quat lastRenderedRotation {};
    float lastRenderedFOV {};

    glm::vec3 currentCameraPosition {};
    glm::quat currentCameraRotation {};
    float currentCameraFOV {};

private:
    // Rendering variables

    glm::ivec2 renderResolution = glm::ivec2(1);

    std::thread::id owningThread; // The id of the thread that owns the asynchronous reprojection framebuffer objects (The thread that needs to render to the framebuffers)
    std::array<GLuint, 3> framebuffers { 0 }; // These are the three framebuffer objects that are used as input to asynchronous reprojection
    std::array<GLenum, 4> drawBuffers = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 }; // These are the buffers that the shader will write to when rendering to one of the three framebuffers (Use glDrawBuffers with this)

    struct BufferMapping
    {
        std::uint8_t display = 0;
        std::uint8_t ready = 1;
        std::uint8_t working = 2;
    } bufferMapping;
    std::uint8_t lastRenderedFrameIndex = 0;

    bool isNewerFrame = false;

    struct BufferLocks
    {
        std::recursive_mutex display {};
        std::recursive_mutex ready {};
        std::recursive_mutex working {};
    } bufferLocks {};

    // These are used to triple buffer the data displayed by asynchronous reprojection
    GLuint latestColorTexture = 0;
    std::array<GLuint, 3> colorTextures {}; //(r, g, b, a)
    std::array<GLuint, 3> positionTextures {}; //(x, y, z) world space
    std::array<GLuint, 3> normalTextures {}; //(x, y, z) camera space
    std::array<GLuint, 3> miscTextures {}; //(material hash 1, motion x, motion y, material hash 2)

private:
    std::shared_ptr<SceneComponent> scene = nullptr;
    int bounces = 2;

    std::unique_ptr<VoxelRenderer> voxelRenderer = nullptr;
    std::unique_ptr<AsyncReprojectionRenderer> reprojection = nullptr;
    std::unique_ptr<PostProcessRenderer> postProcessing = nullptr;
    std::unique_ptr<SVGF> svgf = nullptr;

    static GLuint drawTextureProgram;

    friend class VoxelRenderer;
    friend class AsyncReprojectionRenderer;

    // This is what the reprojection and post processes work on
    std::recursive_mutex outputLock {};
    glm::ivec2 outputResolution {};
    GLuint outputDepthTexture {};
    GLuint outputColorTexture {}; //(r, g, b, a)
    GLuint outputPositionTexture {}; //(x, y, z) world space (This seems to break when toggling reprojection)
    GLuint outputNormalTexture {}; //(x, y, z) camera space
    GLuint outputMiscTexture {}; //(material hash 1, motion x, motion y, material hash 2)

    // Asserts that the calling thread is the owning thread of the framebuffers
    // Will crash on failure
    void isOwningThreadCheck() const;

    void _render(); // This is where all the rendering happens (The underscore is because a publicly facing function that wraps the entire rendering process exists)
    void reproject(float fov = -1); // This is where reprojection occurs
    void postProcess(); // This is where post processing happens
    void finalDisplay(); // Actually displays the image to the screen. Must run after post processing

    void makeOutputTextures();

public:
    Renderer(const std::shared_ptr<Window>& mainContext, const std::shared_ptr<GlfwContext>& offscreenContext);
    ~Renderer();

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

    const glm::ivec2& getRenderResolution();
    void setRenderResolution(glm::ivec2 renderResolution);

    void pollCamera(const std::shared_ptr<CameraComponent>& camera);
    void setScene(const std::shared_ptr<SceneComponent>& scene);
    void setBounces(const int& bounces);

    void render(float fov = -1);

    bool getIsAsynchronousReprojectionEnabled();
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

    std::shared_ptr<PostProcessEffect> addPostProcessEffect(std::shared_ptr<PostProcessEffect> effect);

    glm::vec2 getUpscaleMultiplier();

    glm::vec3 getCurrentCameraPosition();
    glm::quat getCurrentCameraRotation();
    float getCurrentCameraFOV();

    glm::ivec2 getUpscaleResolution();

    void toggleRendering();
    bool isRenderingPaused();

    bool isRenderingAsynchronously();

    void increaseFirstMipMapLevel();
    void decreaseFirstMipMapLevel();
};
