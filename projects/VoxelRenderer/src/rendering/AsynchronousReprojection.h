#pragma once

#include <GL/glew.h>
#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <src/world/Camera.h>

#include <mutex>
#include <vector>
#include <array>

#include "Renderer.h"

// I should probably use a framebuffer, but this needs a custom framebuffer
class AsynchronousReprojection
{
private:

    static GLuint renderProgram;
    static GLuint combineProgram;
    static GLuint combineMaskProgram;

    int currentBuffer = 0;

    //These textures are only used by Asynchronous Reprojection when combining frames
    //As such it only needs old and new versions
    std::array<GLuint, 2> frameCountTextures;

    GLuint combineMaskTextureID;

    glm::ivec2 size;//The size of the offscreen render


    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    GLuint VAO;
    GLuint VBO;
    GLuint EBO;

    void generateMesh(const glm::ivec2& size);

    AsynchronousReprojection();//This is only supposed to be ran by Renderer

    friend class Renderer;

public:

    void setSize(glm::ivec2 size);
    

    GLuint getColorTexture() const;
    GLuint getPositionTexture() const;
    GLuint getMaterialTexture() const;

    void render(const glm::ivec2& reprojectionResolution, const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV, 
        const GLuint& colorTexture, const GLuint& positionTexture);

    
    void combineBuffers(const glm::vec3& lastRenderedCameraPosition, const glm::quat& lastRenderedCameraRotation, const float& lastRenderedCameraFOV,
        const GLuint& oldColorTexture, const GLuint& newColorTexture, const GLuint& oldPositionTexture, const GLuint& newPositionTexture, const GLuint& newMaterialTexture);


};
