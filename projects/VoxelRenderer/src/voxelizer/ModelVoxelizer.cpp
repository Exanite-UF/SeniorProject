#include <src/graphics/ShaderProgram.h>
#include <src/voxelizer/ModelVoxelizer.h>

ModelVoxelizer::~ModelVoxelizer() = default;

std::shared_ptr<Model> ModelVoxelizer::getModel()
{
    return loadedModel;
}

void ModelVoxelizer::loadModel(char* path)
{
    loadedModel = std::make_shared<Model>(path);
}

void ModelVoxelizer::setupBoundingBox()
{
    minBounds = glm::vec3(FLT_MAX);
    maxBounds = glm::vec3(-FLT_MAX);

    for (const Mesh mesh : loadedModel->meshes)
    {
        for (const Vertex vertex : mesh.vertices)
        {
            minBounds = glm::min(minBounds, vertex.Position);
            maxBounds = glm::max(maxBounds, vertex.Position);
        }
    }

    // Padding
    minBounds -= glm::vec3(1.0f);
    maxBounds += glm::vec3(1.0f);

    gridResolution = 16;
    gridSize = glm::ivec3(gridResolution);
    voxelSize = (maxBounds - minBounds) / glm::vec3(gridSize);
    voxelGrid = std::vector<bool>(gridSize.x * gridSize.y * gridSize.z, false);

    printf("BOUNDING BOX SETUP\n");
}

bool ModelVoxelizer::triangleBoxOverlap(const glm::vec3& boxCenter, const glm::vec3& boxHalfSize, const Vertex triVertices[3])
{
    // Seperating Axis Theorem

    // Convert triangle vertices to box space
    glm::vec3 v[3] = {
        triVertices[0].Position - boxCenter,
        triVertices[1].Position - boxCenter,
        triVertices[2].Position - boxCenter
    };

    // Triangle edges
    glm::vec3 e0 = v[1] - v[0];
    glm::vec3 e1 = v[2] - v[1];
    glm::vec3 e2 = v[0] - v[2];

    // Test the 9 separating axes from triangle edges cross box edges
    glm::vec3 boxNormals[3] = {
        glm::vec3(1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, 0, 1)
    };

    for (int i = 0; i < 3; ++i)
    {
        for (int j = 0; j < 3; ++j)
        {
            glm::vec3 axis = cross(e0, boxNormals[j]);
            if (axis == glm::vec3(0))
                continue; // Skip parallel axes

            axis = glm::normalize(axis);
            float triMin, triMax, boxMin, boxMax;
            projectTriangle(axis, v, triMin, triMax);
            projectBox(axis, glm::vec3(0), boxHalfSize, boxMin, boxMax);
            if (!overlaps(triMin, triMax, boxMin, boxMax))
                return false;
        }
    }

    // Test the 3 separating axes from the box faces
    for (int i = 0; i < 3; ++i)
    {
        float triMin, triMax, boxMin, boxMax;
        projectTriangle(boxNormals[i], v, triMin, triMax);
        projectBox(boxNormals[i], glm::vec3(0), boxHalfSize, boxMin, boxMax);
        if (!overlaps(triMin, triMax, boxMin, boxMax))
            return false;
    }

    // Test the triangle normal as separating axis
    glm::vec3 triNormal = cross(e0, e1);
    if (triNormal != glm::vec3(0))
    {
        triNormal = glm::normalize(triNormal);
        float triMin, triMax, boxMin, boxMax;
        projectTriangle(triNormal, v, triMin, triMax);
        projectBox(triNormal, glm::vec3(0), boxHalfSize, boxMin, boxMax);
        if (!overlaps(triMin, triMax, boxMin, boxMax))
            return false;
    }

    // No separating axis found - intersection exists
    return true;
}

bool ModelVoxelizer::triangleIntersection(const Triangle& tri, const glm::vec3& voxelMin)
{
    glm::vec3 boxCenter = voxelMin + voxelSize * 0.5f;
    glm::vec3 boxHalfSize = voxelSize * 0.5f;

    return triangleBoxOverlap(boxCenter, boxHalfSize, tri.vertices);
}

// FIX WITH BVH
void ModelVoxelizer::triangleVoxelization(std::vector<bool>& voxels)
{
    printf("TRIANGLE VOXELIZATION BEGIN\n");

    std::unordered_map<glm::ivec3, std::vector<Triangle>, HashFunction> spatialGrid;
    for (const Triangle& tri : loadedModel->getTriangles())
    {
        glm::ivec3 minCell = worldToGrid(tri.minPoint()); // Convert min point of triangle to grid
        glm::ivec3 maxCell = worldToGrid(tri.maxPoint()); // Convert max point of triangle to grid

        // Iterate over the range of grid cells that the triangle covers
        for (int z = minCell.z; z <= maxCell.z; ++z)
            for (int y = minCell.y; y <= maxCell.y; ++y)
                for (int x = minCell.x; x <= maxCell.x; ++x)
                {
                    // std::cout << z << " " << y << " " << x << std::endl;
                    spatialGrid[{ x, y, z }].push_back(tri); // Store the triangle in the corresponding grid cell
                }
    }

    std::vector<bool> localVoxels(voxels.size(), false);

#pragma omp parallel for collapse(3) schedule(dynamic)
    for (int z = 0; z < gridSize.z; ++z)
    {
        for (int y = 0; y < gridSize.y; ++y)
        {
            for (int x = 0; x < gridSize.x; ++x)
            {
                // std::cout << z << " " << y << " " << x << std::endl;
                glm::vec3 voxelMin = minBounds + glm::vec3(x, y, z) * voxelSize;
                glm::ivec3 gridCell = worldToGrid(voxelMin); //

                // for (const Triangle& tri : loadedModel->getTriangles())
                for (const Triangle& tri : spatialGrid[gridCell])
                {
                    if (triangleIntersection(tri, voxelMin))
                    {
                        localVoxels[z * gridSize.x * gridSize.y + y * gridSize.x + x] = true;
                        // voxelGrid[z * gridSize.x * gridSize.y + y * gridSize.x + x] = true;
                        break;
                    }
                }
            }
        }
    }

// Copy results to global memory
#pragma omp parallel for
    for (size_t i = 0; i < voxels.size(); ++i)
        voxels[i] = localVoxels[i];
    printf("TRIANGLE VOXELIZATION DONE\n");
}

void ModelVoxelizer::raymarchVoxelization(std::vector<bool>& voxels)
{
}

void ModelVoxelizer::voxelizeModel(int option)
{
    printf("VOXELIZING\n");
    if (!loadedModel)
    {
        return;
    }

    setupBoundingBox();

    // triangleVoxelization(voxelGrid);

    generateVoxelMesh();
}

void ModelVoxelizer::generateVoxelMesh()
{
    printf("GENERATING VOXEL MESH\n");

    std::vector<float> cubeVertices = {
        -0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 0.0f, -1.0f, // BACK
        0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f,
        0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, -1.0f,
        -0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f,

        -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, // FRONT
        0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,

        -0.5f, 0.5f, 0.5f, 1.0f, 1.0f, -1.0f, 0.0f, 0.0f, // LEFT
        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 1.0f, 0.0f, -1.0f, 0.0f, 0.0f,

        0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0.0f, 0.0f, // RIGHT
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,

        -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, // bottom
        0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f,
        0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f, -1.0f, 0.0f,
        -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 0.0f, -1.0f, 0.0f,

        -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, // TOP FACE
        0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 0.0f,
        0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
        -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f
    };

    const unsigned int cubeIndices[] = {
        0, 1, 2,
        2, 3, 0,

        4, 5, 6,
        6, 7, 4,

        8, 9, 10,
        10, 11, 8,

        12, 13, 14,
        14, 15, 12,

        16, 17, 18,
        18, 19, 16,

        20, 21, 22,
        22, 23, 20
    };

    // for (int z = 0; z < gridSize.z; ++z) {
    //     for (int y = 0; y < gridSize.y; ++y) {
    //         for (int x = 0; x < gridSize.x; ++x) {
    //
    //            if (!voxelGrid[z * gridSize.x * gridSize.y + y * gridSize.x + x])
    //            {
    //                continue;
    //            }
    //
    //            glm::vec3 voxelCenter = minBounds + glm::vec3(x + 0.5f, y + 0.5f, z + 0.5f) * voxelSize;
    //
    //            // Transform template vertices to world space
    //            for (const auto& index : cubeIndices) {
    //                Vertex v;
    //                v.Position = glm::vec3(cubeVertices[(index * 8)], cubeVertices[(index * 8) + 1], cubeVertices[(index * 8) + 2]);
    //                v.TexCoords = glm::vec2(cubeVertices[(index * 8) + 3], cubeVertices[(index * 8) + 4]);
    //                v.Normal = glm::vec3(cubeVertices[(index * 8) + 5], cubeVertices[(index * 8) + 6], cubeVertices[(index * 8) + 7]);
    //                v.Position = v.Position * voxelSize + voxelCenter;
    //                voxelMesh.push_back(v);
    //            }
    //        }
    //    }
    //}

    // Setup

    activeVoxels.clear();
    activeVoxels.push_back(glm::vec3(0, 0, 0));

    glGenVertexArrays(1, &voxelVAO);
    glGenBuffers(1, &voxelVBO);
    glGenBuffers(1, &voxelEBO);
    glGenBuffers(1, &instanceVBO);

    glBindVertexArray(voxelVAO);

    // Cube VBO (positions)
    glBindBuffer(GL_ARRAY_BUFFER, voxelVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices.data(), GL_STATIC_DRAW);

    // Cube EBO (indices)
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, voxelEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

    // Position attribute (location = 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);

    // Texture Coordinate attribute (location = 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));

    // Normal attribute (location = 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(5 * sizeof(float)));

    // Generate and bind instance VBO for voxel positions
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    // glBufferData(GL_ARRAY_BUFFER, activeVoxels.size() * sizeof(glm::vec3), activeVoxels.data(), GL_DYNAMIC_DRAW);
    glBufferData(GL_ARRAY_BUFFER, activeVoxels.size() * sizeof(glm::vec3), activeVoxels.data(), GL_STATIC_DRAW);

    // Instance Position attribute (location = 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
    glVertexAttribDivisor(1, 1); // Tell OpenGL this is per-instance data

    GLint isEnabled;
    glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &isEnabled);
    std::cout << "Instance attribute enabled: " << isEnabled << std::endl;

    // Unbind VAO

    // activeVoxels.clear();
    // for (int z = 0; z < gridSize.z; ++z) {
    //     for (int y = 0; y < gridSize.y; ++y) {
    //         for (int x = 0; x < gridSize.x; ++x) {
    //             if (voxelGrid[z * gridSize.x * gridSize.y + y * gridSize.x + x]) {
    //                 activeVoxels.emplace_back(x, y, z);
    //             }
    //         }
    //     }
    // }

    // DEBUG

    glBindVertexArray(0);

    isVoxelized = true;
    std::cout << "VOXELIZED!" << std::endl;
}

void ModelVoxelizer::DrawVoxels(const std::shared_ptr<ShaderProgram>& shader, glm::vec3 Position, glm::vec3 Front, glm::vec3 Up, int windowWidth, int windowHeight)
{
    if (!isVoxelized)
    {
        return;
    }

    shader->use();

    // Update instance buffer with active voxel positions
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, activeVoxels.size() * sizeof(glm::vec3), activeVoxels.data(), GL_DYNAMIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, activeVoxels.size() * sizeof(glm::vec3), activeVoxels.data());

    // Camera Setup
    glm::mat4 view = glm::lookAt(Position, Position + Front, Up);
    glm::mat4 projection = glm::perspective(glm::radians(60.0f), (float)windowWidth / (float)windowHeight, 0.001f, 1000.0f);
    glm::mat4 model = glm::mat4(1.0f);

    glUniformMatrix4fv(glGetUniformLocation(shader->programId, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shader->programId, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader->programId, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    //// Material Settings for Phong Shader
    glm::vec3 diffuseColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 specularColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glm::vec3 emissiveColor = glm::vec3(1.0f, 1.0f, 1.0f);
    glUniform3fv(glGetUniformLocation(shader->programId, "defaultMaterial.diffuseColor"), 1, glm::value_ptr(diffuseColor));
    glUniform3fv(glGetUniformLocation(shader->programId, "defaultMaterial.specularColor"), 1, glm::value_ptr(specularColor));
    glUniform3fv(glGetUniformLocation(shader->programId, "defaultMaterial.emissiveColor"), 1, glm::value_ptr(emissiveColor));
    float materialSpecFactor = 0.0f;
    float materialEmisFactor = 0.0f;
    float materialShininess = 32.0f;
    glUniform1f(glGetUniformLocation(shader->programId, "defaultMaterial.specularFactor"), materialSpecFactor);
    glUniform1f(glGetUniformLocation(shader->programId, "defaultMaterial.emissiveFactor"), materialEmisFactor);
    glUniform1f(glGetUniformLocation(shader->programId, "defaultMaterial.shininess"), materialShininess);

    // Render voxels with instancing
    glBindVertexArray(voxelVAO);
    // glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
    glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, activeVoxels.size());
}
