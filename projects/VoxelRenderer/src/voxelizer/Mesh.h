#pragma once

#include <src/utilities/OpenGl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <chrono>
#include <cmath>
#include <fstream>
#include <iostream>
#include <src/graphics/ShaderProgram.h>
#include <sstream>
#include <string>
#include <vector>

struct Vertex
{
    glm::vec3 position {};
    glm::vec3 normal {};
    glm::vec2 uv {};
    glm::vec3 tangent {};
    glm::vec3 bitangent {};
};

struct Triangle
{
    Vertex vertices[3] {};

    glm::vec3 minPoint() const
    {
        glm::vec3 minPoint;
        minPoint.x = std::min({ vertices[0].position.x, vertices[1].position.x, vertices[2].position.x });
        minPoint.y = std::min({ vertices[0].position.y, vertices[1].position.y, vertices[2].position.y });
        minPoint.z = std::min({ vertices[0].position.z, vertices[1].position.z, vertices[2].position.z });
        return minPoint;
    }

    glm::vec3 maxPoint() const
    {
        glm::vec3 maxPoint;
        maxPoint.x = std::max({ vertices[0].position.x, vertices[1].position.x, vertices[2].position.x });
        maxPoint.y = std::max({ vertices[0].position.y, vertices[1].position.y, vertices[2].position.y });
        maxPoint.z = std::max({ vertices[0].position.z, vertices[1].position.z, vertices[2].position.z });
        return maxPoint;
    }
};

// TODO: We have a TextureManager that you can use
// Not sure if we'll end up using this
struct TriangleTexture
{
    unsigned int id {};
    std::string type {};
    std::string path {};
};

class Mesh
{
private:
    unsigned int VAO {};
    unsigned int VBO {};
    unsigned int EBO {};

    void setupMesh();

public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<TriangleTexture> textures;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<TriangleTexture> textures);
    void draw(const std::shared_ptr<ShaderProgram>& shader, glm::vec3 cameraPosition, glm::vec3 cameraForwardDirection, glm::vec3 cameraUpDirection, int windowWidth, int windowHeight);
};
