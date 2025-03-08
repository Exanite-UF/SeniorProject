#pragma once

#include <GL/glew.h>
#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <src/world/Camera.h>
#include <src/gameobjects/GameObject.h>

#include <mutex>
#include <vector>

// I should probably use a framebuffer, but this needs a custom framebuffer
class AsynchronousReprojection
{
private:
    glm::ivec2 size;
    GLuint colorTextureId1;
    GLuint positionTextureId1;
    GLuint materialTextureId1; // Used to combine frames

    GLuint colorTextureId2;
    GLuint positionTextureId2;
    GLuint materialTextureId2;

    GLuint frameCountTextureID1;
    GLuint frameCountTextureID2;

    GLuint combineMaskTextureID;

    int currentFrameBuffer = 0; // This is the framebuffer that the VoxelRenderer renders to

    glm::quat lastCameraRotation;
    glm::vec3 lastCameraPosition;
    float lastCameraFOV;

    static GLuint renderProgram;
    static GLuint combineProgram;
    static GLuint combineMaskProgram;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    GLuint VAO;
    GLuint VBO;
    GLuint EBO;

    void generateMesh();

public:
    AsynchronousReprojection(glm::ivec2 size);

    GLuint getColorTexture() const;
    GLuint getPositionTexture() const;
    GLuint getMaterialTexture() const;

    glm::ivec2 getSize();
    void setSize(glm::ivec2 size);

    void render(GameObject& camera);

    void recordCameraTransform(const glm::vec3& cameraPosition, const glm::quat& cameraRotation, const float& cameraFOV);

    // Uses the mutex to prevent the path tracing from starting until the temporal accumulation is done
    void swapBuffers();

    void combineBuffers();
};
