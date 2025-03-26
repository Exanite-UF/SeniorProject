#include "SVGF.h"

#include <iostream>

#include <src/Content.h>
#include <src/graphics/GraphicsUtility.h>
#include <src/graphics/ShaderManager.h>

GLuint SVGF::integrateFrameProgram {};
GLuint SVGF::firstWaveletIterationProgram {};
GLuint SVGF::waveletIterationProgram {};
GLuint SVGF::toFramebufferProgram {};

void SVGF::isOwningThreadCheck() const
{
    if (std::this_thread::get_id() != frameBufferOwningThread)
    {
        std::string message = "Tried to access the framebuffer from a thread that does not own the framebuffer.";
        std::cout << message << std::endl;
        throw std::runtime_error(message);
    }
}

void SVGF::isLockOwningThreadCheck() const
{
    if (std::this_thread::get_id() != lockOwningThread)
    {
        std::string message = "Tried to access a region of code that requires owning the lock.";
        std::cout << message << std::endl;
        throw std::runtime_error(message);
    }
}

SVGF::SVGF()
{
    integrateFrameProgram = ShaderManager::getInstance().getGraphicsProgram(Content::screenTriVertexShader, Content::integrateFrameFragmentShader);
    firstWaveletIterationProgram = ShaderManager::getInstance().getGraphicsProgram(Content::screenTriVertexShader, Content::firstWaveletIterationFragmentShader);
    waveletIterationProgram = ShaderManager::getInstance().getGraphicsProgram(Content::screenTriVertexShader, Content::waveletIterationFragmentShader);
    toFramebufferProgram = ShaderManager::getInstance().getGraphicsProgram(Content::screenTriVertexShader, Content::toFramebufferSVGFFragmentShader);
}

void SVGF::remakeTextures()
{
    //Create input textures
    {
        // Create color texture
        glDeleteTextures(1, &colorInputTexture);
        glGenTextures(1, &colorInputTexture);
        glBindTexture(GL_TEXTURE_2D, colorInputTexture);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create motion texture
        glDeleteTextures(1, &motionInputTexture);
        glGenTextures(1, &motionInputTexture);
        glBindTexture(GL_TEXTURE_2D, motionInputTexture);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGBA, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create normal texture
        glDeleteTextures(1, &normalInputTexture);
        glGenTextures(1, &normalInputTexture);
        glBindTexture(GL_TEXTURE_2D, normalInputTexture);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create position texture
        glDeleteTextures(1, &positionInputTexture);
        glGenTextures(1, &positionInputTexture);
        glBindTexture(GL_TEXTURE_2D, positionInputTexture);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    

    //Create internal textures
    {
        //Make moment history textures
        for(int i = 0; i < 2; i++){
            // Create moment 1 texture
            glDeleteTextures(1, &moment1HistoryTexture[i]);
            glGenTextures(1, &moment1HistoryTexture[i]);
            glBindTexture(GL_TEXTURE_2D, moment1HistoryTexture[i]);
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
            }
            glBindTexture(GL_TEXTURE_2D, 0);

            // Create moment 2 texture
            glDeleteTextures(1, &moment2HistoryTexture[i]);
            glGenTextures(1, &moment2HistoryTexture[i]);
            glBindTexture(GL_TEXTURE_2D, moment2HistoryTexture[i]);
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
            }
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        

        // Create color texture
        glDeleteTextures(1, &colorHistoryTexture);
        glGenTextures(1, &colorHistoryTexture);
        glBindTexture(GL_TEXTURE_2D, colorHistoryTexture);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        // Create normal texture
        glDeleteTextures(1, &normalHistoryTexture);
        glGenTextures(1, &normalHistoryTexture);
        glBindTexture(GL_TEXTURE_2D, normalHistoryTexture);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        
        // Create position texture
        glDeleteTextures(1, &positionHistoryTexture);
        glGenTextures(1, &positionHistoryTexture);
        glBindTexture(GL_TEXTURE_2D, positionHistoryTexture);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        //Make temp textures

        // Create clip position texture
        glDeleteTextures(1, &clipPositionTexture);
        glGenTextures(1, &clipPositionTexture);
        glBindTexture(GL_TEXTURE_2D, clipPositionTexture);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);

        for(int i = 0; i < 2; i++){
            // Create moment 1 texture
            glDeleteTextures(1, &tempColorTexture[i]);
            glGenTextures(1, &tempColorTexture[i]);
            glBindTexture(GL_TEXTURE_2D, tempColorTexture[i]);
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
            }
            glBindTexture(GL_TEXTURE_2D, 0);

            // Create moment 2 texture
            glDeleteTextures(1, &tempVarianceTexture[i]);
            glGenTextures(1, &tempVarianceTexture[i]);
            glBindTexture(GL_TEXTURE_2D, tempVarianceTexture[i]);
            {
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, this->renderResolution.x, this->renderResolution.y, 0, GL_RGB, GL_FLOAT, nullptr);
            }
            glBindTexture(GL_TEXTURE_2D, 0);
        }
    }
}

void SVGF::makeFramebuffer()
{
    glDeleteFramebuffers(1, &inputFramebuffer);
    glGenFramebuffers(1, &inputFramebuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, inputFramebuffer);

    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, colorInputTexture, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, positionInputTexture, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, normalInputTexture, 0);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, motionInputTexture, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    frameBufferOwningThread = std::this_thread::get_id();

    glViewport(0, 0, renderResolution.x, renderResolution.y); // Adjust the viewport of the offscreen context
}

GLuint SVGF::getFramebuffer()
{
    isOwningThreadCheck();

    return inputFramebuffer;
}

std::array<GLenum, 4> SVGF::getDrawBuffer() const
{
    return drawBuffer;
}

void SVGF::setRenderResolution(glm::ivec2 size)
{
    if(this->renderResolution == size){
        return;
    }

    this->renderResolution = size;
    remakeTextures();
}

void SVGF::integrateFrame(const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV, const glm::vec3& cameraMovement)
{
    isLockOwningThreadCheck();
    //Integrate the frame
    {
        glUseProgram(integrateFrameProgram);

        //Bind textures
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, colorInputTexture);
    
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, motionInputTexture);
    
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, positionInputTexture);
    
    
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, colorHistoryTexture);
    
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, positionHistoryTexture);
    
            if(activeMomentTexture % 2 == 0){
                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_2D, moment1HistoryTexture[0]);
    
                glActiveTexture(GL_TEXTURE6);
                glBindTexture(GL_TEXTURE_2D, moment2HistoryTexture[0]);
            }else{
                glActiveTexture(GL_TEXTURE5);
                glBindTexture(GL_TEXTURE_2D, moment1HistoryTexture[1]);
    
                glActiveTexture(GL_TEXTURE6);
                glBindTexture(GL_TEXTURE_2D, moment2HistoryTexture[1]);
            }
        }
        
        //Make framebuffer
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        //Bind the output textures
        {
            if(activeTempBuffer % 2 == 0){
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tempColorTexture[0], 0);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, tempVarianceTexture[0], 0);
            }else{
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tempColorTexture[1], 0);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, tempVarianceTexture[1], 0);
            }

            if(activeMomentTexture % 2 == 0){
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, moment1HistoryTexture[1], 0);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, moment2HistoryTexture[1], 0);
            }else{
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, moment1HistoryTexture[0], 0);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, moment2HistoryTexture[0], 0);
            }

            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, clipPositionTexture, 0);
        }

        const GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };


        glUniform3fv(glGetUniformLocation(integrateFrameProgram, "cameraPosition"), 1, glm::value_ptr(cameraPosition));
        glUniform4f(glGetUniformLocation(integrateFrameProgram, "cameraRotation"), cameraRotation.x, cameraRotation.y, cameraRotation.z, cameraRotation.w);
        glUniform3fv(glGetUniformLocation(integrateFrameProgram, "cameraMovement"), 1, glm::value_ptr(cameraMovement));
        glUniform2i(glGetUniformLocation(integrateFrameProgram, "resolution"), this->renderResolution.x, this->renderResolution.y);

        //Run the shader
        {
            glEnable(GL_DEPTH_TEST);
            glDepthMask(GL_TRUE);  // Ensures depth is written to the depth buffer
            GLuint emptyVertexArray;
            glGenVertexArrays(1, &emptyVertexArray);
    
            glBindVertexArray(emptyVertexArray);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Tell opengl to draw to the texture in the framebuffer
            glDrawBuffers(5, buffers);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glDisable(GL_BLEND);
            glBindVertexArray(0);

            glDeleteVertexArrays(1, &emptyVertexArray);
            glDepthMask(GL_FALSE); 
            glDisable(GL_DEPTH_TEST);
        }

        //Delete the frame buffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &framebuffer);

        //Unbind textures
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
    
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
    
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, 0);
    
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, 0);
    
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, 0);
    
            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, 0);
    
            glActiveTexture(GL_TEXTURE6);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        
        activeMomentTexture++;//The moments have been updated, so change the active buffer

        glUseProgram(0);
    }

    //Perform first wavelet integration
    {
        
        glUseProgram(firstWaveletIterationProgram);

        //Bind textures
        {
            if(activeTempBuffer % 2 == 0){
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tempColorTexture[0]);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, tempVarianceTexture[0]);
            }else{
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tempColorTexture[1]);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, tempVarianceTexture[1]);
            }

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, clipPositionTexture);

            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, normalInputTexture);

            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, motionInputTexture);

            glActiveTexture(GL_TEXTURE5);
            glBindTexture(GL_TEXTURE_2D, positionInputTexture);
        }

        //Make framebuffer
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        //Bind the output textures
        {
            if(activeTempBuffer % 2 == 0){
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tempColorTexture[1], 0);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, tempVarianceTexture[1], 0);
            }else{
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tempColorTexture[0], 0);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, tempVarianceTexture[0], 0);
            }

            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, colorHistoryTexture, 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, normalHistoryTexture, 0);
            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, positionHistoryTexture, 0);
        }

        const GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4 };

        glUniform2i(glGetUniformLocation(firstWaveletIterationProgram, "resolution"), this->renderResolution.x, this->renderResolution.y);
        glUniform1f(glGetUniformLocation(firstWaveletIterationProgram, "cameraFovTan"), std::tan(cameraFOV * 0.5));

        //Run the shader
        {
            GLuint emptyVertexArray;
            glGenVertexArrays(1, &emptyVertexArray);
    
            glBindVertexArray(emptyVertexArray);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Tell opengl to draw to the texture in the framebuffer
            glDrawBuffers(5, buffers);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glDisable(GL_BLEND);
            glBindVertexArray(0);

            glDeleteVertexArrays(1, &emptyVertexArray);
        }

        //Delete the frame buffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &framebuffer);

        //Unbind textures
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
    
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
    
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, 0);
    
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, 0);
    
            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        
        activeTempBuffer++;//The temp buffer have been updated to their next valid state, so change the active buffer

        glUseProgram(0);
    }
}

void SVGF::display(const GLuint& framebuffer, const std::array<GLenum, 4>& drawBuffers, int iterations, const float& cameraFOV)
{
    isLockOwningThreadCheck();
    for(int i = 0; i < iterations; i++){
        glUseProgram(waveletIterationProgram);

        //Bind textures
        {
            if(activeTempBuffer % 2 == 0){
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tempColorTexture[0]);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, tempVarianceTexture[0]);
            }else{
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tempColorTexture[1]);

                glActiveTexture(GL_TEXTURE1);
                glBindTexture(GL_TEXTURE_2D, tempVarianceTexture[1]);
            }

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, clipPositionTexture);

            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, normalHistoryTexture);

            glActiveTexture(GL_TEXTURE4);
            glBindTexture(GL_TEXTURE_2D, motionInputTexture);
        }

        //Make framebuffer
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        //Bind the output textures
        {
            if(activeTempBuffer % 2 == 0){
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tempColorTexture[1], 0);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, tempVarianceTexture[1], 0);
            }else{
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, tempColorTexture[0], 0);
                glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, tempVarianceTexture[0], 0);
            }
        }

        const GLenum buffers[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };

        glUniform2i(glGetUniformLocation(waveletIterationProgram, "resolution"), this->renderResolution.x, this->renderResolution.y);
        glUniform1f(glGetUniformLocation(waveletIterationProgram, "cameraFovTan"), std::tan(cameraFOV * 0.5));
        glUniform1i(glGetUniformLocation(waveletIterationProgram, "iteration"), i + 1);

        //Run the shader
        {
            GLuint emptyVertexArray;
            glGenVertexArrays(1, &emptyVertexArray);
    
            glBindVertexArray(emptyVertexArray);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Tell opengl to draw to the texture in the framebuffer
            glDrawBuffers(2, buffers);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glDisable(GL_BLEND);
            glBindVertexArray(0);

            glDeleteVertexArrays(1, &emptyVertexArray);
        }

        //Delete the frame buffer
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &framebuffer);

        //Unbind textures
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);
    
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);
    
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, 0);
    
            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, 0);
        }
        
        activeTempBuffer++;//The temp buffer have been updated to their next valid state, so change the active buffer

        glUseProgram(0);
    }

    //std::cout << "SVGF display" << std::endl;
    //Display to the framebuffer
    {
        glUseProgram(toFramebufferProgram);

        //Bind textures
        {
            if(activeTempBuffer % 2 == 0){
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tempColorTexture[0]);

                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, tempVarianceTexture[0]);
            }else{
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, tempColorTexture[1]);

                glActiveTexture(GL_TEXTURE4);
                glBindTexture(GL_TEXTURE_2D, tempVarianceTexture[1]);
            }

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, positionHistoryTexture);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, normalHistoryTexture);

            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, motionInputTexture);
        }


        //Run the shader
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);        
        {
            GLuint emptyVertexArray;
            glGenVertexArrays(1, &emptyVertexArray);
    
            glBindVertexArray(emptyVertexArray);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            // Tell opengl to draw to the texture in the framebuffer
            glDrawBuffers(4, drawBuffers.data());
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glDisable(GL_BLEND);
            glBindVertexArray(0);

            glDeleteVertexArrays(1, &emptyVertexArray);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        //unbind textures
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, 0);

            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, 0);

            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, 0);

            glActiveTexture(GL_TEXTURE3);
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glUseProgram(0);
    }
}

void SVGF::unlock()
{
    mtx.unlock();
    lockOwningThread = std::thread::id();//Default constructed id, doesn't correspond to any thread
}

void SVGF::lock()
{
    mtx.lock();
    lockOwningThread = std::this_thread::get_id();
}
