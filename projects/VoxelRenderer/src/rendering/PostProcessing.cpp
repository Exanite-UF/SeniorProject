#include "PostProcessing.h"
#include <iostream>

#include <src/Content.h>
#include <src/graphics/ShaderManager.h>
#include <src/graphics/GraphicsUtility.h>


GLuint PostProcessing::drawTextureProgram;
std::unordered_map<std::string, GLuint> PostProcessing::programs;

std::unordered_map<std::string, std::shared_ptr<PostProcess>> PostProcess::existingProcesses;

void PostProcess::bindTextures(GLuint previousOutputTexture, GLuint colorTexture, GLuint positionTexture, GLuint normalTexture)
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, previousOutputTexture);

    if(colorTextureBinding != GL_TEXTURE0){
        glActiveTexture(colorTextureBinding);
        glBindTexture(GL_TEXTURE_2D, colorTexture);
    }

    if(positionTextureBinding != GL_TEXTURE0){
        glActiveTexture(positionTextureBinding);
        glBindTexture(GL_TEXTURE_2D, positionTexture);
    }

    if(normalTextureBinding != GL_TEXTURE0){
        glActiveTexture(normalTextureBinding);
        glBindTexture(GL_TEXTURE_2D, normalTexture);
    }
}

void PostProcess::unbindTextures()
{
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);

    if(colorTextureBinding != GL_TEXTURE0){
        glActiveTexture(colorTextureBinding);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if(positionTextureBinding != GL_TEXTURE0){
        glActiveTexture(positionTextureBinding);
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    if(normalTextureBinding != GL_TEXTURE0){
        glActiveTexture(normalTextureBinding);
        glBindTexture(GL_TEXTURE_2D, 0);
    }
}

void PostProcess::applyProcess(GLuint currentOutput, GLuint previousOutputTexture, GLuint colorTexture, GLuint positionTexture, GLuint normalTexture)
{
    GLuint framebuffer;
    glGenFramebuffers(1, &framebuffer);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, currentOutput, 0);

    //Bind the textures
    bindTextures(previousOutputTexture, colorTexture, positionTexture, normalTexture);

    setUniforms(program);

    //Do the draw
    glBindVertexArray(GraphicsUtility::getEmptyVertexArray());

    //Tell opengl to draw to the texture in the framebuffer
    GLenum drawBuffer = GL_COLOR_ATTACHMENT0;
    glDrawBuffers(1, &drawBuffer);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glBindVertexArray(0);

    unbindTextures();

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &framebuffer);
}

void PostProcess::preventDuplicateBindings()
{
    static std::string message = "Cannot bind multiple textures to the same location.";
    std::unordered_set<GLenum> usedBindings;
    

    if(colorTextureBinding != GL_TEXTURE0){
        //This first one can never error
        //if(usedBindings.count(colorTextureBinding)){
        //    std::cout << message << std::endl;
        //    throw std::runtime_error(message);
        //}
        usedBindings.insert(colorTextureBinding);
    }

    if(positionTextureBinding != GL_TEXTURE0){
        if(usedBindings.count(positionTextureBinding)){
            std::cout << message << std::endl;
            throw std::runtime_error(message);
        }
        usedBindings.insert(positionTextureBinding);
    }

    if(normalTextureBinding != GL_TEXTURE0){
        if(usedBindings.count(normalTextureBinding)){
            std::cout << message << std::endl;
            throw std::runtime_error(message);
        }
        //The last one never contributes to a check
        //usedBindings.insert(normalTextureBinding);
    }
}

PostProcess::PostProcess(GLuint program, GLenum colorTextureBinding, GLenum positionTextureBinding, GLenum normalTextureBinding)
{
    this->program = program;
    this->colorTextureBinding = colorTextureBinding;
    this->positionTextureBinding = positionTextureBinding;
    this->normalTextureBinding = normalTextureBinding;

    preventDuplicateBindings();
}

std::shared_ptr<PostProcess> PostProcess::makeNewPostProcess(std::string name, GLuint program, GLenum colorTextureBinding, GLenum positionTextureBinding, GLenum normalTextureBinding)
{
    if(existingProcesses.count(name)){
        std::string message = "Cannot make a new post processing effect with the same name as an existing one.";
        std::cout << message << std::endl;
        throw std::runtime_error(message);
    }

    std::shared_ptr<PostProcess> result = std::shared_ptr<PostProcess>(new PostProcess(program, colorTextureBinding, positionTextureBinding, normalTextureBinding));

    existingProcesses[name] = result;

    return result;
}

void PostProcessing::applyProcess(std::size_t processID, GLuint colorTexture, GLuint positionTexture, GLuint normalTexture)
{

    GLuint outputTexture;
    GLuint inputTexture;

    //These names do not correspond with the getOutputTexture value, since they are the names of the textures as is relevant to this shader, rather than post processing as whole
    outputTexture = renderTextures[(currentTexture + 1) % 2];
    inputTexture = renderTextures[currentTexture % 2];

    postProcesses[processID]->applyProcess(outputTexture, inputTexture, colorTexture, positionTexture, normalTexture);

    currentTexture = (currentTexture + 1) % 2;
}

void PostProcessing::makeTextures()
{
    //Remake the color texture
    glDeleteTextures(2, renderTextures.data());
    glGenTextures(2, renderTextures.data());

    for(int i = 0; i < 2; i++){
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

PostProcessing::PostProcessing()
{
    drawTextureProgram = ShaderManager::getInstance().getGraphicsProgram(Content::screenTriVertexShader, Content::drawTextureFragmentShader);
}

void PostProcessing::applyAllProcesses(const glm::ivec2& outputResolution, GLuint colorTexture, GLuint positionTexture, GLuint normalTexture)
{
    if(this->outputResolution != outputResolution){
        this->outputResolution = outputResolution;
        makeTextures();
    }

    //Before any processes should run, the color texture should replace the current output texture

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

        glUseProgram(0);
    }
    
    glBindTexture(GL_TEXTURE_2D, 0);



    for(int i = 0; i < postProcesses.size(); i++){
        applyProcess(i, colorTexture, positionTexture, normalTexture);
    }
}

GLuint PostProcessing::getOutputTexture()
{
    return renderTextures[currentTexture % 2];
}
