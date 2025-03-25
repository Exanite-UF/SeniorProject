#pragma once

#include "Renderer.h"

#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <mutex>
#include <vector>

#include <src/utilities/OpenGl.h>
#include <src/world/CameraComponent.h>

class SVGF
{
private:
    static GLuint integrateFrameProgram;
    static GLuint firstWaveletIterationProgram;
    static GLuint waveletIterationProgram;
    static GLuint toFramebufferProgram;

    //Inputs
    GLuint colorInputTexture = 0;//The raw color result from the path trace step
    GLuint motionInputTexture = 0;//Contains the motion vectors and roughness for each pixel
    GLuint normalInputTexture = 0;//The screen space normals
    GLuint positionInputTexture = 0;//The worldspace position of the pixel

    //The position will need to be transformed into camera space


    //Internal
    int activeMomentTexture = 0;//Determines which of the two moment histories to use
    std::array<GLuint, 2> moment1HistoryTexture = { 0 };//Stores a rolling average of color
    std::array<GLuint, 2> moment2HistoryTexture = { 0 };//Stores a rolling average of color squared
    GLuint colorHistoryTexture = 0;//This is filtered, so it is not the same as the rolling average
    GLuint normalHistoryTexture = 0;
    GLuint positionHistoryTexture = 0;

    //These are needed for the wavelet integration
    int activeTempBuffer = 0;
    GLuint depthTexture = 0;//This one doesn't change with wavelet iterations
    std::array<GLuint, 2> tempColorTexture = { 0 };
    std::array<GLuint, 2> tempVarianceTexture = { 0 };

    //moment 1 and 2 need 2 textures to swap between
    //Integrating a frame requires reading from and writing to moments 1 and 2


    std::thread::id lockOwningThread;
    std::recursive_mutex mtx;


    std::thread::id frameBufferOwningThread; // The id of the thread that owns the framebuffer (The thread that needs to render to the framebuffer)
    GLuint inputFramebuffer = 0;//A frame buffer that other things write to
    std::array<GLenum, 4> drawBuffer = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 };//These are the things that get written to

    // Asserts that the calling thread is the owning thread of the framebuffers
    // Will crash on failure
    void isOwningThreadCheck() const;

    void isLockOwningThreadCheck() const;

    SVGF();// This is only supposed to be run by Renderer

    friend class Renderer;

    glm::ivec2 renderResolution;

    void remakeTextures();

public:

    

    // This needs to be called on the thread that needs to render to the asynchronous reprojection input
    void makeFramebuffer();

    GLuint getFramebuffer();
    std::array<GLenum, 4> getDrawBuffer() const;
    
    void setRenderResolution(glm::ivec2 size);


    
    //Assumes that new data is present in the input
    //It combines it with the internal data
    //Requires owning the lock
    void integrateFrame(const glm::vec3& cameraPosition, const glm::quat& cameraRotation);

    //Performs the requested number of additional wavelet iterations, then renders to the framebuffer
    //Requires owning the lock
    void display(const GLuint& framebuffer, const std::array<GLenum, 4>& drawBuffers, int iterations);

    //Locks everything
    void lock();
    void unlock();
};
