#pragma once

#include <src/utilities/OpenGl.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cmath>
#include <chrono>

#include <src/voxelizer/Shader.h>

struct Vertex
{
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoords;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
};

struct Triangle
{
    Vertex vertices[3];

    glm::vec3 minPoint() const {
        glm::vec3 minPoint;
        minPoint.x = std::min({vertices[0].Position.x, vertices[1].Position.x, vertices[2].Position.x});
        minPoint.y = std::min({vertices[0].Position.y, vertices[1].Position.y, vertices[2].Position.y});
        minPoint.z = std::min({vertices[0].Position.z, vertices[1].Position.z, vertices[2].Position.z});
        return minPoint;
    }

    glm::vec3 maxPoint() const {
        glm::vec3 maxPoint;
        maxPoint.x = std::max({vertices[0].Position.x, vertices[1].Position.x, vertices[2].Position.x});
        maxPoint.y = std::max({vertices[0].Position.y, vertices[1].Position.y, vertices[2].Position.y});
        maxPoint.z = std::max({vertices[0].Position.z, vertices[1].Position.z, vertices[2].Position.z});
        return maxPoint;
    }
};

// Not sure if we'll end up using this
struct TriangleTexture
{
    unsigned int id;
    std::string type;
    std::string path;
};

class Mesh
{
private:
    unsigned int VAO, VBO, EBO;
    void setupMesh();
public:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;
    std::vector<TriangleTexture> textures;

    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices, std::vector<TriangleTexture> textures);
    void Draw(Shader &shader);
};
