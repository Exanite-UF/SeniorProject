#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <glm/vec2.hpp>

#include <src/utilities/OpenGl.h>

class PostProcessEffect;
class Renderer;

// For all post processing shader, the following must be included in the fragment shader
// layout(binding = 0) uniform sampler2D inputTexture;
// out vec4 out_color;

// Additionally all post processing shaders use ScreenTri.vertex.glsl for the vertex shader

class PostProcessRenderer
{
private:
    static GLuint drawTextureProgram;

    glm::ivec2 outputResolution {};
    std::array<GLuint, 2> renderTextures {}; // Post processing must flip between the two textures inorder to work (input texture may not be the output texture)

    static std::unordered_map<std::string, GLuint> programs;

    std::vector<std::shared_ptr<PostProcessEffect>> postProcesses {};

    int currentTexture = 0;

    void applyProcess(std::size_t processID, GLuint colorTexture, GLuint positionTexture, GLuint normalTexture, GLuint materialTexture);

    void makeTextures();

    PostProcessRenderer();

    friend class Renderer;

public:
    void applyAllProcesses(const glm::ivec2& outputResolution, GLuint colorTexture, GLuint positionTexture, GLuint normalTexture, GLuint materialTexture);

    GLuint getOutputTexture();

    void addPostProcessEffect(std::shared_ptr<PostProcessEffect> effect);

    bool hasAnyProcesses();
};

class PostProcessEffect
{
protected:
    GLuint program = 0;

private:
    static std::unordered_map<std::string, std::shared_ptr<PostProcessEffect>> existingProcesses;

    // These are the texture binding locations in glsl
    // If non-positive, it means the texture is not used (The previous output is always bound to location 0)
    GLenum colorTextureBinding {};
    GLenum positionTextureBinding {};
    GLenum normalTextureBinding {};
    GLenum materialTextureBinding {};

    void bindTextures(GLuint previousOutputTexture, GLuint colorTexture, GLuint positionTexture, GLuint normalTexture, GLuint materialTexture);
    void unbindTextures();

    void applyProcess(GLuint currentOutput, GLuint previousOutputTexture, GLuint colorTexture, GLuint positionTexture, GLuint normalTexture, GLuint materialTexture);

    friend class PostProcessRenderer;

    // This will throw upon finding duplicate bindings
    void preventDuplicateBindings();

    PostProcessEffect(GLuint program, GLenum colorTextureBinding = GL_TEXTURE0, GLenum positionTextureBinding = GL_TEXTURE0, GLenum normalTextureBinding = GL_TEXTURE0, GLenum materialTextureBinding = GL_TEXTURE0);

public:
    static std::shared_ptr<PostProcessEffect> getPostProcess(std::string name, GLuint program = 0, GLenum colorTextureBinding = GL_TEXTURE0, GLenum positionTextureBinding = GL_TEXTURE0, GLenum normalTextureBinding = GL_TEXTURE0, GLenum materialTextureBinding = GL_TEXTURE0);

    // Takes the program as input
    std::function<void(GLuint)> setUniforms = [](GLuint program) {};
};
