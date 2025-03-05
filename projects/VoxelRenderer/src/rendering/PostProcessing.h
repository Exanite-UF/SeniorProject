#pragma once

#include <array>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <functional>
#include <memory>


#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/vec2.hpp>

class PostProcess;

class PostProcessing{
private:

    static GLuint drawTextureProgram;

    glm::ivec2 outputResolution;
    std::array<GLuint, 2> renderTextures;//Post processing must flip between the two textures inorder to work (input texture may not be the output texture)

    static std::unordered_map<std::string, GLuint> programs;

    std::vector<std::shared_ptr<PostProcess>> postProcesses;

    int currentTexture = 0;

    void applyProcess(std::size_t processID, GLuint colorTexture, GLuint positionTexture, GLuint normalTexture);
    
    void makeTextures();

public:
    PostProcessing();
    
    void applyAllProcesses(const glm::ivec2& outputResolution, GLuint colorTexture, GLuint positionTexture, GLuint normalTexture);

    GLuint getOutputTexture();
};

class PostProcess{
protected:
    GLuint program = 0;

private:
    static std::unordered_map<std::string, std::shared_ptr<PostProcess>> existingProcesses;

    //These are the texture binding locations in glsl
    //If non-positive, it means the texture is not used (The previous output is always bound to location 0)
    GLenum colorTextureBinding;
    GLenum positionTextureBinding;
    GLenum normalTextureBinding;

    void bindTextures(GLuint previousOutputTexture, GLuint colorTexture, GLuint positionTexture, GLuint normalTexture);
    void unbindTextures();

    void applyProcess(GLuint currentOutput, GLuint previousOutputTexture, GLuint colorTexture, GLuint positionTexture, GLuint normalTexture);

    friend class PostProcessing;

    //This will throw upon finding duplicate bindings
    void preventDuplicateBindings();

    PostProcess(GLuint program, GLenum colorTextureBinding = GL_TEXTURE0, GLenum positionTextureBinding = GL_TEXTURE0, GLenum normalTextureBinding = GL_TEXTURE0);

public:

    static std::shared_ptr<PostProcess> makeNewPostProcess(std::string name, GLuint program, GLenum colorTextureBinding = GL_TEXTURE0, GLenum positionTextureBinding = GL_TEXTURE0, GLenum normalTextureBinding = GL_TEXTURE0);

    //Takes the program as input
    std::function<void(GLuint)> setUniforms = [](GLuint program){};
};