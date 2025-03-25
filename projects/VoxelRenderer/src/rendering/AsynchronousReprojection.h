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

// I should probably use a framebuffer, but this needs a custom framebuffer
class AsyncReprojectionRenderer
{
private:
    static GLuint renderProgram;
    static GLuint combineProgram;
    static GLuint combine2Program;
    static GLuint denoiseProgram;

    int currentBuffer = 0;

    glm::ivec2 size {}; // The size of the offscreen render

    std::vector<float> vertices {};
    std::vector<unsigned int> indices {};

    GLuint VAO {};
    GLuint VBO {};
    GLuint EBO {};

    GLuint tempColorTexture;//This stores the color temporarily since frame combining is a two step process
    GLuint tempVarianceTexture;//This stores the color temporarily since frame combining is a two step process

    void generateMesh(const glm::ivec2& size);

    AsyncReprojectionRenderer(); // This is only supposed to be ran by Renderer

    friend class Renderer;

    void _denoise(int iteration, const GLuint& colorTexture, const GLuint& varianceTexture, const GLuint& outputColorTexture, const GLuint& outputVarianceTexture, const GLuint& positionTexture, const GLuint& normalTexture, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV);

public:
    void setSize(glm::ivec2 size);//This sets the render resolution that is expected as input

    GLuint getColorTexture() const;
    GLuint getPositionTexture() const;
    GLuint getMiscTexture() const;

    void render(GLuint framebuffer, const glm::ivec2& reprojectionResolution, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV,
        const GLuint& colorTexture, const GLuint& positionTexture, const GLuint& normalTexture, const GLuint& miscTexture);

    void combineBuffers(const GLuint& latestColorTexture, const GLuint& oldColorTexture, const GLuint& newMiscTexture, const GLuint& newColorTexture, const GLuint& oldColorSquaredTexture, const GLuint& newColorSquaredTexture,
        const GLuint& oldPositionTexture, const GLuint& newPositionTexture, const GLuint& normalTexture, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV, const GLuint& varianceTexture);

   void denoise(const GLuint& colorTexture, const GLuint& varianceTexture, const GLuint& positionTexture, const GLuint& normalTexture, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV);
};
