#include "Renderer.h"

#include <tracy/Tracy.hpp>

#include <algorithm>
#include <iostream>
#include <string>

#include <src/Content.h>
#include <src/graphics/GraphicsUtility.h>
#include <src/graphics/ShaderManager.h>
#include <src/windowing/Window.h>

GLuint Renderer::drawTextureProgram {};

void Renderer::offscreenRenderingFunc()
{
    tracy::SetThreadName("Offscreen rendering");

    offscreenContext->makeContextCurrent();

    while (_isRenderingOffscreen)
    {
        const char* frameId = "Render offscreen";
        FrameMarkStart(frameId);

        _render();

        FrameMarkEnd(frameId);
    }
}

void Renderer::isOwningThreadCheck() const
{
    if (std::this_thread::get_id() != owningThread)
    {
        std::string message = "Tried to access the asynchronous framebuffer from a thread that does not own the framebuffer.";
        std::cout << message << std::endl;
        throw std::runtime_error(message);
    }
}

Renderer::Renderer(const std::shared_ptr<Window>& mainContext, const std::shared_ptr<GlfwContext>& offscreenContext)
{
    drawTextureProgram = ShaderManager::getInstance().getGraphicsProgram(Content::screenTriVertexShader, Content::drawTextureFragmentShader);

    this->mainContext = mainContext;
    this->offscreenContext = offscreenContext;

    voxelRenderer = std::unique_ptr<VoxelRenderer>(new VoxelRenderer());
    reprojection = std::unique_ptr<AsyncReprojectionRenderer>(new AsyncReprojectionRenderer());
    postProcessing = std::unique_ptr<PostProcessRenderer>(new PostProcessRenderer());
    svgf = std::unique_ptr<SVGF>(new SVGF());
}

Renderer::~Renderer()
{
    stopAsynchronousReprojection();
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
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, miscTextures[i], 0);
        // glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, colorSquaredTextures[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    owningThread = std::this_thread::get_id();
    isSizeDirtyThread = false;

    //This shouldn't be here (The offscreen context already sets the view port)
    //glViewport(0, 0, renderResolution.x, renderResolution.y); // Adjust the viewport of the offscreen context
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
        lastRenderedFrameIndex = bufferMapping.display;

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
    // reprojection->combineBuffers(colorTextures[lastRenderedFrameIndex], colorTextures[bufferMapping.ready], miscTextures[bufferMapping.ready], positionTextures[lastRenderedFrameIndex], positionTextures[bufferMapping.ready], lastRenderedPosition);

    lastRenderedFrameIndex = bufferMapping.ready;

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

const glm::ivec2& Renderer::getRenderResolution()
{
    return renderResolution;
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
        glDeleteTextures(1, &miscTextures[i]);
        glGenTextures(1, &miscTextures[i]);
        glBindTexture(GL_TEXTURE_2D, miscTextures[i]);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_GREATER);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGBA, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    isNewerFrame = false;
    reprojection->setRenderResolution(renderResolution);
    voxelRenderer->setResolution(renderResolution);
    svgf->setRenderResolution(renderResolution);

    isSizeDirtyThread = true;
}

void Renderer::pollCamera(const std::shared_ptr<CameraComponent>& camera)
{
    std::scoped_lock lock(cameraMtx);
    currentCameraPosition = camera->getTransform()->getGlobalPosition();
    currentCameraRotation = camera->getTransform()->getGlobalRotation();
    currentCameraFOV = camera->getHorizontalFov();
}

void Renderer::setScene(const std::shared_ptr<SceneComponent>& scene)
{
    this->scene = scene;
}

void Renderer::setBounces(const int& bounces)
{
    this->bounces = bounces;
}

void Renderer::render(float fov)
{
    const char* frameId = "Render to screen";
    FrameMarkStart(frameId);

    if (!_isRenderingOffscreen)
    {
        _render();
    }

    reproject(fov);
    postProcess();

    finalDisplay();

    FrameMarkEnd(frameId);
}

void Renderer::_render()
{
    if(_isRenderingPaused){
        return;
    }

    if (isSizeDirtyThread)
    {
        makeFramebuffers();
        svgf->makeFramebuffer();
    }

    {
        glViewport(0, 0, renderResolution.x, renderResolution.y);
        std::scoped_lock lock(cameraMtx);

        if (_isRenderingOffscreen)
        {
            lastRenderedFOV += overdrawFOV;
            lastRenderedFOV = std::min(lastRenderedFOV, maxFov);
        }

        voxelRenderer->prepareRayTraceFromCamera(currentCameraPosition, currentCameraRotation, currentCameraFOV);
        {
            std::shared_lock lockScene(scene->getMutex());

            voxelRenderer->executePathTrace(scene->getAllChunks(), bounces, lastRenderedPosition, lastRenderedRotation, lastRenderedFOV);
        }

        // This need SVGF's framebuffer
        // voxelRenderer->render(getWorkingFramebuffer(), drawBuffers, currentCameraPosition, currentCameraRotation, currentCameraFOV);
        voxelRenderer->render(svgf->getFramebuffer(), svgf->getDrawBuffer(), currentCameraPosition, currentCameraRotation, currentCameraFOV);

        // SVGF
        svgf->lock();
        svgf->integrateFrame(currentCameraPosition, currentCameraRotation, currentCameraFOV, currentCameraPosition - lastRenderedPosition);
        svgf->display(getWorkingFramebuffer(), drawBuffers, 4, currentCameraFOV);
        svgf->unlock();

        olderRenderedPosition = lastRenderedPosition;
        lastRenderedPosition = currentCameraPosition;
        lastRenderedRotation = currentCameraRotation;
        lastRenderedFOV = currentCameraFOV;
    }

    glFinish();
    swapWorkingBuffer();
    renderCount++;
}

void Renderer::reproject(float fov)
{
    std::scoped_lock lock(cameraMtx, bufferLocks.display, outputLock);

    if (fov < 0)
    {
        fov = currentCameraFOV;
    }

    // This repolling of the output size happens here, because things that use the result of the new data, don't actually need the new data, until the reprojection resolution changes.
    int width, height;
    glfwGetWindowSize(mainContext->getGlfwWindowHandle(), &width, &height);

    if (glm::ivec2(width, height) != outputResolution)
    {
        outputResolution = glm::ivec2(width, height);
        upscaleMultiplier = { (float)width / renderResolution.x, (float)height / renderResolution.y };

        makeOutputTextures();
    }

    glViewport(0, 0, width, height);

    swapDisplayBuffer();

    // make the framebuffer
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, outputColorTexture, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, outputPositionTexture, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, outputNormalTexture, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, outputMiscTexture, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, outputDepthTexture, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // Do the render

    if(_isRenderingOffscreen || _isRenderingPaused){
        reprojection->render(framebuffer, glm::ivec2(width, height), currentCameraPosition, currentCameraRotation, fov, colorTextures[bufferMapping.display], positionTextures[bufferMapping.display], normalTextures[bufferMapping.display], miscTextures[bufferMapping.display]);
    }else{
        reprojection->bypass(framebuffer, glm::ivec2(width, height), colorTextures[bufferMapping.display]);
    }
    

    // Delete the framebuffer
    glDeleteFramebuffers(1, &framebuffer);

    reprojectionCount++;
}

void Renderer::postProcess()
{
    std::scoped_lock lock(outputLock, bufferLocks.display);

    // Do the post processing
    if (postProcessing->hasAnyProcesses())
    {
        postProcessing->applyAllProcesses(this->outputResolution, outputColorTexture, outputPositionTexture, outputNormalTexture, outputMiscTexture);
    }
}

void Renderer::finalDisplay()
{
    std::scoped_lock lock(outputLock);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(drawTextureProgram);

    glActiveTexture(GL_TEXTURE0);
    if (postProcessing->hasAnyProcesses())
    {
        glBindTexture(GL_TEXTURE_2D, postProcessing->getOutputTexture());
    }
    else
    {
        glBindTexture(GL_TEXTURE_2D, outputColorTexture);
    }

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(GraphicsUtility::getEmptyVertexArray());
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::makeOutputTextures()
{
    // Remake the color texture
    glDeleteTextures(1, &outputColorTexture);
    glGenTextures(1, &outputColorTexture);
    glBindTexture(GL_TEXTURE_2D, outputColorTexture);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->outputResolution.x, this->outputResolution.y, 0, GL_RGBA, GL_FLOAT, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // Remake the position texture
    glDeleteTextures(1, &outputPositionTexture);
    glGenTextures(1, &outputPositionTexture);
    glBindTexture(GL_TEXTURE_2D, outputPositionTexture);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->outputResolution.x, this->outputResolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // Remake the normal texture
    glDeleteTextures(1, &outputNormalTexture);
    glGenTextures(1, &outputNormalTexture);
    glBindTexture(GL_TEXTURE_2D, outputNormalTexture);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->outputResolution.x, this->outputResolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // Remake the depth texture
    glDeleteTextures(1, &outputDepthTexture);
    glGenTextures(1, &outputDepthTexture);
    glBindTexture(GL_TEXTURE_2D, outputDepthTexture);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, this->outputResolution.x, this->outputResolution.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, 0);

    // Remake the misc texture
    glDeleteTextures(1, &outputMiscTexture);
    glGenTextures(1, &outputMiscTexture);
    glBindTexture(GL_TEXTURE_2D, outputMiscTexture);
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->outputResolution.x, this->outputResolution.y, 0, GL_RGBA, GL_FLOAT, nullptr);
    }
    glBindTexture(GL_TEXTURE_2D, 0);
}

bool Renderer::getIsAsynchronousReprojectionEnabled()
{
    return _isRenderingOffscreen;
}

void Renderer::startAsynchronousReprojection()
{
    _isRenderingOffscreen = true;
    isSizeDirtyThread = true;
    offscreenThread = std::thread(&Renderer::offscreenRenderingFunc, this);
}

void Renderer::stopAsynchronousReprojection()
{
    _isRenderingOffscreen = false;
    if (offscreenThread.joinable())
    {
        offscreenThread.join();
    }
    isSizeDirtyThread = true;
}

void Renderer::toggleAsynchronousReprojection()
{
    if (_isRenderingOffscreen)
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

std::shared_ptr<PostProcessEffect> Renderer::addPostProcessEffect(std::shared_ptr<PostProcessEffect> effect)
{
    postProcessing->addPostProcessEffect(effect);
    return effect;
}

glm::vec2 Renderer::getUpscaleMultiplier()
{
    return upscaleMultiplier;
}

glm::vec3 Renderer::getCurrentCameraPosition()
{
    return currentCameraPosition;
}

glm::quat Renderer::getCurrentCameraRotation()
{
    return currentCameraRotation;
}

float Renderer::getCurrentCameraFOV()
{
    return currentCameraFOV;
}

glm::ivec2 Renderer::getUpscaleResolution()
{
    return outputResolution;
}

void Renderer::toggleRendering()
{
    _isRenderingPaused = !_isRenderingPaused;
}

bool Renderer::isRenderingPaused()
{
    return _isRenderingPaused;
}

bool Renderer::isRenderingAsynchronously()
{
    return _isRenderingOffscreen;
}
