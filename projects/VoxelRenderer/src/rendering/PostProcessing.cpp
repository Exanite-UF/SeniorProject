#include "PostProcessing.h"
#include <iostream>

#include <src/Content.h>
#include <src/graphics/GraphicsUtility.h>
#include <src/graphics/ShaderManager.h>
#include <tracy/Tracy.hpp>

GLuint PostProcessRenderer::drawTextureProgram {};
std::unordered_map<std::string, GLuint> PostProcessRenderer::programs {};

std::unordered_map<std::string, std::shared_ptr<PostProcessEffect>> PostProcessEffect::existingProcesses {};

void PostProcessEffect::bindTextures(GLuint previousOutputTexture, GLuint colorTexture, GLuint positionTexture, GLuint normalTexture, GLuint materialTexture)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, previousOutputTexture);

    if (colorTextureBinding != GL_TEXTURE0)
    {
        glActiveTexture(colorTextureBinding);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
    }

    if (positionTextureBinding != GL_TEXTURE0)
    {
        glActiveTexture(positionTextureBinding);
        glBindTexture(GL_TEXTURE_2D, positionTexture);
    }

    if (normalTextureBinding != GL_TEXTURE0)
    {
        glActiveTexture(normalTextureBinding);
        glBindTexture(GL_TEXTURE_2D, normalTexture);
    }

    if (materialTextureBinding != GL_TEXTURE0)
    {
        glActiveTexture(materialTextureBinding);
        glBindTexture(GL_TEXTURE_2D, materialTexture);
    }
}

void PostProcessEffect::unbindTextures()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if (colorTextureBinding != GL_TEXTURE0)
    {
        glActiveTexture(colorTextureBinding);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (positionTextureBinding != GL_TEXTURE0)
    {
        glActiveTexture(positionTextureBinding);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (normalTextureBinding != GL_TEXTURE0)
    {
        glActiveTexture(normalTextureBinding);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if (materialTextureBinding != GL_TEXTURE0)
    {
        glActiveTexture(materialTextureBinding);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void PostProcessEffect::applyProcess(GLuint currentOutput, GLuint previousOutputTexture, GLuint colorTexture, GLuint positionTexture, GLuint normalTexture, GLuint materialTexture)
{
    ZoneScoped;

    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, currentOutput, 0);

    glUseProgram(program);

    // Bind the textures
    // std::cout << previousOutputTexture << std::endl;
    bindTextures(previousOutputTexture, colorTexture, positionTexture, normalTexture, materialTexture);

    setUniforms(program);

    // Do the draw
    glBindVertexArray(GraphicsUtility::getEmptyVertexArray());

    // Tell opengl to draw to the texture in the framebuffer
    GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuffer);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);

    unbindTextures();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &framebuffer);

    glUseProgram(0);
}

void PostProcessEffect::preventDuplicateBindings()
{
    ZoneScoped;

    static std::string message = "Cannot bind multiple textures to the same location.";
    std::unordered_set<GLenum> usedBindings;

    if (colorTextureBinding != GL_TEXTURE0)
    {
        // This first one can never error
        // if(usedBindings.count(colorTextureBinding)){
        //     std::cout << message << std::endl;
        //     throw std::runtime_error(message);
        // }
        usedBindings.insert(colorTextureBinding);
    }

    if (positionTextureBinding != GL_TEXTURE0)
    {
        if (usedBindings.count(positionTextureBinding))
        {
            std::cout << message << std::endl;
            throw std::runtime_error(message);
        }
        usedBindings.insert(positionTextureBinding);
    }

    if (normalTextureBinding != GL_TEXTURE0)
    {
        if (usedBindings.count(normalTextureBinding))
        {
            std::cout << message << std::endl;
            throw std::runtime_error(message);
        }
        usedBindings.insert(normalTextureBinding);
    }

    if (materialTextureBinding != GL_TEXTURE0)
    {
        if (usedBindings.count(materialTextureBinding))
        {
            std::cout << message << std::endl;
            throw std::runtime_error(message);
        }
        // The last one never contributes to a check
        // usedBindings.insert(normalTextureBinding);
    }
}

PostProcessEffect::PostProcessEffect(GLuint program, GLenum colorTextureBinding, GLenum positionTextureBinding, GLenum normalTextureBinding, GLenum materialTextureBinding)
{
    this->program = program;
    this->colorTextureBinding = colorTextureBinding;
    this->positionTextureBinding = positionTextureBinding;
    this->normalTextureBinding = normalTextureBinding;
    this->materialTextureBinding = materialTextureBinding;

    preventDuplicateBindings();
}

std::shared_ptr<PostProcessEffect> PostProcessEffect::getEffect(std::string name, GLuint program, GLenum colorTextureBinding, GLenum positionTextureBinding, GLenum normalTextureBinding, GLenum materialTextureBinding)
{
    ZoneScoped;

    if (existingProcesses.count(name))
    {
        return existingProcesses[name];
    }

    if (program == 0)
    {
        std::string message = "Post Process effect must provide a shader program.";
        std::cout << message << std::endl;
        throw std::runtime_error(message);
    }

    std::shared_ptr<PostProcessEffect> result = std::shared_ptr<PostProcessEffect>(new PostProcessEffect(program, colorTextureBinding, positionTextureBinding, normalTextureBinding, materialTextureBinding));

    existingProcesses[name] = result;

    return result;
}

void PostProcessRenderer::applyProcess(std::size_t processID, GLuint colorTexture, GLuint positionTexture, GLuint normalTexture, GLuint materialTexture)
{
    ZoneScoped;

    GLuint outputTexture;
    GLuint inputTexture;

    // These names do not correspond with the getOutputTexture value, since they are the names of the textures as is relevant to this shader, rather than post processing as whole
    outputTexture = renderTextures[(currentTexture + 1) % 2];
    inputTexture = renderTextures[currentTexture % 2];

    postProcesses[processID]->applyProcess(outputTexture, inputTexture, colorTexture, positionTexture, normalTexture, materialTexture);

    currentTexture = (currentTexture + 1) % 2;
}

void PostProcessRenderer::makeTextures()
{
    ZoneScoped;

    // Remake the color texture
    glDeleteTextures(2, renderTextures.data());
    glGenTextures(2, renderTextures.data());

    for (int i = 0; i < 2; i++)
    {
        glBindTexture(GL_TEXTURE_2D, renderTextures[i]);
        {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, this->outputResolution.x, this->outputResolution.y, 0, GL_RGBA, GL_FLOAT, nullptr);
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

PostProcessRenderer::PostProcessRenderer()
{
    drawTextureProgram = ShaderManager::getInstance().getGraphicsProgram(Content::screenTriVertexShader, Content::drawTextureFragmentShader)->programId;
}

void PostProcessRenderer::applyAllProcesses(const glm::ivec2& outputResolution, GLuint colorTexture, GLuint positionTexture, GLuint normalTexture, GLuint materialTexture)
{
    ZoneScoped;

    if (this->outputResolution != outputResolution)
    {
        this->outputResolution = outputResolution;
        makeTextures();
    }

    // Before any processes should run, the color texture should replace the current output texture

    {
        GLuint framebuffer;
        glGenFramebuffers(1, &framebuffer);

        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderTextures[currentTexture % 2], 0);

        glUseProgram(drawTextureProgram);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, colorTexture);

        glBindVertexArray(GraphicsUtility::getEmptyVertexArray());
        GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
        glDrawBuffers(1, &drawBuffer);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &framebuffer);

        glBindTexture(GL_TEXTURE_2D, 0);

        glUseProgram(0);
    }

    // Now apply the effects
    for (int i = 0; i < postProcesses.size(); i++)
    {
        // std::cout << "HI"  << i << std::endl;
        applyProcess(i, colorTexture, positionTexture, normalTexture, materialTexture);
    }
}

GLuint PostProcessRenderer::getOutputTexture()
{
    return renderTextures[currentTexture % 2];
}

void PostProcessRenderer::addPostProcessEffect(std::shared_ptr<PostProcessEffect> effect)
{
    postProcesses.push_back(effect);
}

bool PostProcessRenderer::hasAnyProcesses()
{
    return this->postProcesses.size() > 0;
}
