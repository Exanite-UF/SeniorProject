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
    static GLuint combineMaskProgram;

    int currentBuffer = 0;

    // These textures are only used by Asynchronous Reprojection when combining frames
    // As such it only needs old and new versions
    std::array<GLuint, 2> frameCountTextures {};

    GLuint combineMaskTextureID {};

    glm::ivec2 size {}; // The size of the offscreen render

    std::vector<float> vertices {};
    std::vector<unsigned int> indices {};

    GLuint VAO {};
    GLuint VBO {};
    GLuint EBO {};

    void generateMesh(const glm::ivec2& size);

    AsyncReprojectionRenderer(); // This is only supposed to be ran by Renderer

    friend class Renderer;

public:
    void setSize(glm::ivec2 size);

    GLuint getColorTexture() const;
    GLuint getPositionTexture() const;
    GLuint getMaterialTexture() const;

    void render(GLuint framebuffer, const glm::ivec2& reprojectionResolution, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV,
        const GLuint& colorTexture, const GLuint& positionTexture, const GLuint& normalTexture, const GLuint& materialTexture);

    void combineBuffers(const glm::vec3& cameraMovement, const glm::vec3& lastRenderedCameraPosition, const glm::quat& lastRenderedCameraRotation, const float& lastRenderedCameraFOV,
        const GLuint& oldColorTexture, const GLuint& newColorTexture, const GLuint& oldPositionTexture, const GLuint& newPositionTexture, const GLuint& newMaterialTexture, const GLuint& newNormalTexture);
};
