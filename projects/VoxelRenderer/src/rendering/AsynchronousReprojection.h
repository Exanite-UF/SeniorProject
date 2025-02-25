#pragma once

#include <GL/glew.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/common.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <src/world/Camera.h>

#include <vector>

//I should probably use a framebuffer, but this needs a custom framebuffer
class AsynchronousReprojection{
 private:
    glm::ivec2 size;
    GLuint framebufferId;
    GLuint colorTextureId;
    GLuint positionTextureId;

    glm::quat lastCameraRotation;
    glm::vec3 lastCameraPosition;

    static GLuint renderProgram;

    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    GLuint VAO;
    GLuint VBO;
    GLuint EBO;

    void generateMesh();

 public:
    AsynchronousReprojection(glm::ivec2 size);

    GLuint getFrameBufferId() const;

    glm::ivec2 getSize();
    void setSize(glm::ivec2 size);

    void render(const Camera& camera);

    void recordCameraTransform(const Camera& camera);
};