#include "Renderer.h"

#include <algorithm>
#include <iostream>
#include <string>

#include <src/windowing/Window.h>

void Renderer::offscreenRenderingFunc()
{
    glfwMakeContextCurrent(offscreenContext);

    while (isRenderingOffscreen)
    {
        _render();
    }
}

void Renderer::isOwningThreadCheck()
{
    if (std::this_thread::get_id() != owningThread)
    {
        std::string message = "Tried to access the asynchronous framebuffer from a thread that does not own the framebuffer.";
        std::cout << message << std::endl;
        throw std::runtime_error(message);
    }
}

Renderer::Renderer(GLFWwindow* mainContext, GLFWwindow* offscreenContext)
{
    this->mainContext = mainContext;
    this->offscreenContext = offscreenContext;

    voxelRenderer = std::unique_ptr<VoxelRenderer>(new VoxelRenderer());
    reprojection = std::unique_ptr<AsynchronousReprojection>(new AsynchronousReprojection());
}

void Renderer::makeFramebuffers()
{
    std::scoped_lock lock(bufferLocks.display, bufferLocks.ready, bufferLocks.working);
    // std::cout << "H" << std::endl;

    // Delete the three framebuffers
    glDeleteFramebuffers(3, framebuffers.data());

    // Make the three framebuffers
    glGenFramebuffers(3, framebuffers.data());

    // Set up the framebuffers
    for (int i = 0; i < 3; i++)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffers[i]);

        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorTextures[i], 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, positionTextures[i], 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, normalTextures[i], 0);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, materialTextures[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    owningThread = std::this_thread::get_id();
    isSizeDirtyThread = false;

    glViewport(0, 0, renderResolution.x, renderResolution.y); // Adjust the viewport of the offscreen context
}

void Renderer::swapDisplayBuffer()
{
    // If a newer frame exists, then swap the display buffer for the newest completed frame
    if (isNewerFrame)
    {

        // This will block if it is unable to lock both the ready and working buffers
        // It will unlock upon destruction
        std::scoped_lock lock(bufferLocks.display, bufferLocks.ready);

        std::swap(bufferMapping.display, bufferMapping.ready);

        isNewerFrame = false;
    }
}

void Renderer::swapWorkingBuffer()
{
    isOwningThreadCheck();

    // This will block if it is unable to lock both the ready and working buffers
    // It will unlock upon destruction
    std::scoped_lock lock(bufferLocks.ready, bufferLocks.working);

    std::swap(bufferMapping.ready, bufferMapping.working);

    reprojection->combineBuffers(lastRenderedPosition, lastRenderedRotation, lastRenderedFOV,
        colorTextures[bufferMapping.display], colorTextures[bufferMapping.ready],
        positionTextures[bufferMapping.display], positionTextures[bufferMapping.ready],
        materialTextures[bufferMapping.ready]);

    isNewerFrame = true;
}

GLuint Renderer::getWorkingFramebuffer()
{
    isOwningThreadCheck();

    // This will block if it is unable to lock both the ready and working buffers
    // It will unlock upon destruction
    std::scoped_lock lock(bufferLocks.working);

    return framebuffers[bufferMapping.working];
}

void Renderer::setRenderResolution(glm::ivec2 renderResolution)
{
    if (this->renderResolution == renderResolution)
    {
        return;
    }

    glFinish(); // Ensure that all the current rendering tasks are done

    this->renderResolution = renderResolution;

    // Lock all the buffers
    std::scoped_lock lock(bufferLocks.display, bufferLocks.ready, bufferLocks.working);

    // std::cout << renderResolution.x << " " << renderResolution.y << std::endl;
    // Remake all the render textures
    for (int i = 0; i < 3; i++)
    {
        // Create color texture
        glDeleteTextures(1, &colorTextures[i]);
        glGenTextures(1, &colorTextures[i]);
        glBindTexture(GL_TEXTURE_2D, colorTextures[i]);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGBA, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create position texture
        glDeleteTextures(1, &positionTextures[i]);
        glGenTextures(1, &positionTextures[i]);
        glBindTexture(GL_TEXTURE_2D, positionTextures[i]);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create material texture
        glDeleteTextures(1, &normalTextures[i]);
        glGenTextures(1, &normalTextures[i]);
        glBindTexture(GL_TEXTURE_2D, normalTextures[i]);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create material texture
        glDeleteTextures(1, &materialTextures[i]);
        glGenTextures(1, &materialTextures[i]);
        glBindTexture(GL_TEXTURE_2D, materialTextures[i]);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    isNewerFrame = false;
    reprojection->setSize(renderResolution);
    voxelRenderer->setResolution(renderResolution);
    voxelRenderer->setRaysPerPixel(1);

    isSizeDirtyThread = true;
}

void Renderer::pollCamera(const Camera& camera)
{
    std::scoped_lock lock(cameraMtx);
    currentCameraPosition = camera.transform.getGlobalPosition();
    currentCameraRotation = camera.transform.getGlobalRotation();
    currentCameraFOV = camera.getHorizontalFov();
}

void Renderer::setScene(Scene& scene)
{
    this->scene = &scene;
}

void Renderer::setBounces(const int& bounces)
{
    this->bounces = bounces;
}

void Renderer::render(float fov)
{
    if (!isRenderingOffscreen)
    {
        _render();
    }

    reproject(fov);
}

void Renderer::_render()
{
    if (isSizeDirtyThread)
    {
        makeFramebuffers();
    }

    {
        glViewport(0, 0, renderResolution.x, renderResolution.y);
        std::scoped_lock lock(cameraMtx);

        lastRenderedPosition = currentCameraPosition;
        lastRenderedRotation = currentCameraRotation;
        lastRenderedFOV = currentCameraFOV;

        if (isRenderingOffscreen)
        {
            lastRenderedFOV += overdrawFOV;
            lastRenderedFOV = std::min(lastRenderedFOV, maxFov);
        }

        voxelRenderer->prepareRayTraceFromCamera(lastRenderedPosition, lastRenderedRotation, lastRenderedFOV);

        voxelRenderer->executePathTrace(scene->worlds, bounces);

        voxelRenderer->render(getWorkingFramebuffer(), drawBuffers, lastRenderedPosition, lastRenderedRotation, lastRenderedFOV);
    }

    glFinish();
    swapWorkingBuffer();
    renderCount++;
}

void Renderer::reproject(float fov)
{
    std::scoped_lock lock(cameraMtx, bufferLocks.display);

    if (fov < 0)
    {
        fov = currentCameraFOV;
    }

    int width, height;
    glfwGetWindowSize(mainContext, &width, &height);

    glViewport(0, 0, width, height);

    reprojectionResolutionMultiplier = { (float)width / renderResolution.x, (float)height / renderResolution.y };

    swapDisplayBuffer();
    reprojection->render(glm::ivec2(width, height), currentCameraPosition, currentCameraRotation, fov, colorTextures[bufferMapping.display], positionTextures[bufferMapping.display]);
    reprojectionCount++;
}

void Renderer::startAsynchronousReprojection()
{
    isRenderingOffscreen = true;
    isSizeDirtyThread = true;
    offscreenThread = std::thread(&Renderer::offscreenRenderingFunc, this);
}

void Renderer::stopAsynchronousReprojection()
{
    isRenderingOffscreen = false;
    if (offscreenThread.joinable())
    {
        offscreenThread.join();
    }
    isSizeDirtyThread = true;
}

void Renderer::toggleAsynchronousReprojection()
{
    if (isRenderingOffscreen)
    {
        stopAsynchronousReprojection();
    }
    else
    {
        startAsynchronousReprojection();
    }
}

int Renderer::getRenderCounter()
{
    return renderCount;
}

void Renderer::resetRenderCounter()
{
    renderCount = 0;
}

int Renderer::getReprojectionCounter()
{
    return reprojectionCount;
}

void Renderer::resetReprojectionCounter()
{
    reprojectionCount = 0;
}

void Renderer::setAsynchronousOverdrawFOV(float extraFOV)
{
    overdrawFOV = extraFOV;
}
