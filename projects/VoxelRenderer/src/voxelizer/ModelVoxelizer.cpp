#include <src/gameobjects/GameObject.h>
#include <src/gameobjects/TransformComponent.h>
#include <src/graphics/ShaderProgram.h>
#include <src/utilities/OpenGl.h>
#include <src/voxelizer/ModelVoxelizer.h>
#include <src/windowing/GlfwContext.h>
#include <src/windowing/Window.h>
#include <src/world/MaterialManager.h>
#include <src/world/VoxelChunkCommandBuffer.h>
#include <src/world/VoxelChunkManager.h>

#include <random>

// Helper
bool isExtensionSupported(const char* extensionName)
{
    GLint numExtensions;
    glGetIntegerv(GL_NUM_EXTENSIONS, &numExtensions);

    for (GLint i = 0; i < numExtensions; ++i)
    {
        const char* ext = reinterpret_cast<const char*>(glGetStringi(GL_EXTENSIONS, i));
        if (std::strcmp(ext, extensionName) == 0)
        {
            return true;
        }
    }
    return false;
}

ModelVoxelizer::~ModelVoxelizer() = default;

std::shared_ptr<Model> ModelVoxelizer::getModel()
{
    return loadedModel;
}

std::shared_ptr<VoxelChunkData> ModelVoxelizer::getChunkData()
{
    return chunkData;
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
            minBounds = glm::min(minBounds, vertex.position);
            maxBounds = glm::max(maxBounds, vertex.position);
        }
    }

    // Padding
    minBounds -= glm::vec3(1.0f);
    maxBounds += glm::vec3(1.0f);

    // Orthographic Projection Matrices
    projectionX = glm::ortho(minBounds.y, maxBounds.y, minBounds.z, maxBounds.z, minBounds.x, maxBounds.x);
    projectionY = glm::ortho(minBounds.x, maxBounds.x, minBounds.z, maxBounds.z, minBounds.y, maxBounds.y);
    projectionZ = glm::ortho(minBounds.x, maxBounds.x, minBounds.y, maxBounds.y, minBounds.z, maxBounds.z);

    viewX = glm::lookAt(glm::vec3(maxBounds.x, 0.0f, 0.0f), glm::vec3(minBounds.x, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    viewY = glm::lookAt(glm::vec3(0.0f, maxBounds.y, 0.0f), glm::vec3(0.0f, minBounds.y, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    viewZ = glm::lookAt(glm::vec3(0.0f, 0.0f, maxBounds.z), glm::vec3(0.0f, 0.0f, minBounds.z), glm::vec3(0.0f, 1.0f, 0.0f));

    glm::vec3 bounds = maxBounds - minBounds;
    std::cout << "Bounds: (" << bounds.x << ", " << bounds.y << ", " << bounds.z << ")" << std::endl;
    float maxDim = std::max({ bounds.x, bounds.y, bounds.z });

    if (maxDim < 1e-6f)
        maxDim = 1.0f;
    glm::vec3 normalizedDims = (bounds / maxDim) * (float)gridResolution;
    gridSize = glm::ivec3(
        std::ceil(normalizedDims.x),
        std::ceil(normalizedDims.y),
        std::ceil(normalizedDims.z));

    voxelSize = bounds / glm::vec3(gridSize);
    voxelGrid = std::vector<bool>(gridSize.x * gridSize.y * gridSize.z, false);

    printf("BOUNDING BOX SETUP\n");
}

bool ModelVoxelizer::triangleBoxOverlap(const glm::vec3& boxCenter, const glm::vec3& boxHalfSize, const Vertex triVertices[3])
{
    // Separating Axis Theorem

    // Convert triangle vertices to box space
    glm::vec3 v[3] = {
        triVertices[0].position - boxCenter,
        triVertices[1].position - boxCenter,
        triVertices[2].position - boxCenter
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

void ModelVoxelizer::triangleVoxelization(std::vector<bool>& voxels)
{
    ZoneScoped;

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

    std::vector localVoxels(voxels.size(), false);

#pragma omp parallel for collapse(3) schedule(dynamic)
    for (int z = 0; z < gridSize.z; ++z)
    {
        for (int y = 0; y < gridSize.y; ++y)
        {
            for (int x = 0; x < gridSize.x; ++x)
            {
                glm::vec3 voxelMin = minBounds + glm::vec3(x, y, z) * voxelSize;

                glm::ivec3 gridCell = worldToGrid(voxelMin);

                for (const Triangle& tri : spatialGrid[gridCell])
                {
                    if (triangleIntersection(tri, voxelMin))
                    {
                        localVoxels[z * gridSize.x * gridSize.y + y * gridSize.x + x] = true;
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

void ModelVoxelizer::setupModelForRasterization()
{
    ZoneScoped;

    glGenTextures(1, &voxelTexture);
    glGenVertexArrays(1, &modelVAO);
    glGenBuffers(1, &modelVBO);
    glGenBuffers(1, &modelEBO);

    // Cube Texture
    glBindTexture(GL_TEXTURE_3D, voxelTexture);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R32UI, gridSize.x, gridSize.y, gridSize.z, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "OpenGL Error: " << error << std::endl;
    }
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    // Counter
    int modelVertexCount = 0;

#pragma omp parallel for
    for (size_t i = 0; i < loadedModel->meshes.size(); ++i)
    {
        const Mesh& mesh = loadedModel->meshes[i];

#pragma omp critical
        {
            vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
            std::transform(mesh.indices.begin(), mesh.indices.end(), std::back_inserter(indices),
                [modelVertexCount](unsigned int index)
                {
                    return index + modelVertexCount;
                });
            modelVertexCount += mesh.vertices.size();
        }
    }

    glBindVertexArray(modelVAO);

    glBindBuffer(GL_ARRAY_BUFFER, modelVBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(VertexPositionUvNormal), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPositionUvNormal), reinterpret_cast<void*>(offsetof(VertexPositionUvNormal, position)));

    glBindVertexArray(0);

    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR)
    {
        std::cerr << "OpenGL Error during setupModelForRasterization: " << err << std::endl;
    }
}

void ModelVoxelizer::renderModelForRasterization()
{
    glBindVertexArray(modelVAO);

    // Issue the draw call
    // Bind the 3D texture for writing
    glBindImageTexture(0, voxelTexture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);
    glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    glBindVertexArray(0);
}

void ModelVoxelizer::performConservativeRasterization()
{
    ZoneScoped;

    setupModelForRasterization();

    glEnable(GL_CONSERVATIVE_RASTERIZATION_NV);
    glDisable(GL_DEPTH_TEST);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

    rasterizationShader->use();

    GLint gridSizeLoc = glGetUniformLocation(rasterizationShader->programId, "gridSize");
    GLint minBoundsLoc = glGetUniformLocation(rasterizationShader->programId, "minBounds");
    GLint maxBoundsLoc = glGetUniformLocation(rasterizationShader->programId, "maxBounds");

    glUniform3fv(gridSizeLoc, 1, glm::value_ptr(glm::vec3(gridSize)));
    glUniform3fv(minBoundsLoc, 1, glm::value_ptr(minBounds));
    glUniform3fv(maxBoundsLoc, 1, glm::value_ptr(maxBounds));
    if (gridSizeLoc == -1 || minBoundsLoc == -1 || maxBoundsLoc == -1)
    {
        std::cerr << "Error: Failed to get uniform location for one or more uniforms." << std::endl;
    }

    // Render from the X-axis
    glUniformMatrix4fv(glGetUniformLocation(rasterizationShader->programId, "projection"), 1, GL_FALSE, glm::value_ptr(projectionX));
    glUniformMatrix4fv(glGetUniformLocation(rasterizationShader->programId, "view"), 1, GL_FALSE, glm::value_ptr(viewX));

    renderModelForRasterization();

    // Render from the Y-axis
    glUniformMatrix4fv(glGetUniformLocation(rasterizationShader->programId, "projection"), 1, GL_FALSE, glm::value_ptr(projectionY));
    glUniformMatrix4fv(glGetUniformLocation(rasterizationShader->programId, "view"), 1, GL_FALSE, glm::value_ptr(viewY));
    renderModelForRasterization();

    // Render from the Z-axis
    glUniformMatrix4fv(glGetUniformLocation(rasterizationShader->programId, "projection"), 1, GL_FALSE, glm::value_ptr(projectionZ));
    glUniformMatrix4fv(glGetUniformLocation(rasterizationShader->programId, "view"), 1, GL_FALSE, glm::value_ptr(viewZ));
    renderModelForRasterization();

    std::vector<unsigned int> voxelData(gridSize.x * gridSize.y * gridSize.z);

    glBindTexture(GL_TEXTURE_3D, voxelTexture);
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, voxelData.data());
    std::cout << "Voxel (0, 0, 0): " << voxelData[0] << std::endl;

    glDisable(GL_CONSERVATIVE_RASTERIZATION_NV);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "OpenGL Error: " << error << std::endl;
    }

    voxelGrid.clear();
    voxelGrid.resize(gridSize.x * gridSize.y * gridSize.z, false);

// Map the 1D texture data to the 3D voxel grid
#pragma omp parallel for collapse(3) schedule(dynamic)
    for (int z = 0; z < gridSize.z; ++z)
    {
        for (int y = 0; y < gridSize.y; ++y)
        {
            for (int x = 0; x < gridSize.x; ++x)
            {
                int index = z * (gridSize.x * gridSize.y) + y * gridSize.x + x;
                voxelGrid[index] = (voxelData[index] > 0);
            }
        }
    }

    glEnable(GL_DEPTH_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void ModelVoxelizer::performRayMarchingVoxelization()
{
    ZoneScoped;

    // Gather Triangles
    std::vector<Triangle> triangles = loadedModel->getTriangles();
    std::vector<glm::vec3> triangleVertices;
    triangleVertices.reserve(triangles.size() * 3);

    for (const Triangle& tri : triangles)
    {
        triangleVertices.push_back(tri.vertices[0].position);
        triangleVertices.push_back(tri.vertices[1].position);
        triangleVertices.push_back(tri.vertices[2].position);
    }

    // Generate Buffer Objects
    glGenBuffers(1, &triangleSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, triangleSSBO);
    glBufferData(GL_SHADER_STORAGE_BUFFER, triangleVertices.size() * sizeof(glm::vec3), triangleVertices.data(), GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, triangleSSBO);

    glGenTextures(1, &voxelTexture);
    glBindTexture(GL_TEXTURE_3D, voxelTexture);
    glTexStorage3D(GL_TEXTURE_3D, 1, GL_R32UI, gridSize.x, gridSize.y, gridSize.z);
    GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "OpenGL Error: " << error << std::endl;
    }
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glBindImageTexture(0, voxelTexture, 0, GL_TRUE, 0, GL_READ_WRITE, GL_R32UI);

    rayMarchingShader->use();

    int triCount = static_cast<int>(triangleVertices.size() / 3);
    glUniform1i(glGetUniformLocation(rayMarchingShader->programId, "triCount"), triCount);

    GLint gridSizeLoc = glGetUniformLocation(rayMarchingShader->programId, "gridSize");
    GLint minBoundsLoc = glGetUniformLocation(rayMarchingShader->programId, "minBounds");
    GLint maxBoundsLoc = glGetUniformLocation(rayMarchingShader->programId, "maxBounds");

    glUniform3fv(gridSizeLoc, 1, glm::value_ptr(glm::vec3(gridSize)));
    glUniform3fv(minBoundsLoc, 1, glm::value_ptr(minBounds));
    glUniform3fv(maxBoundsLoc, 1, glm::value_ptr(maxBounds));
    if (gridSizeLoc == -1 || minBoundsLoc == -1 || maxBoundsLoc == -1)
    {
        std::cerr << "Error: Failed to get uniform location for one or more uniforms." << std::endl;
    }

    glm::ivec3 dispatch = (gridSize + glm::ivec3(7)) / glm::ivec3(8); // Round up to the nearest multiple of 8
    glDispatchCompute(dispatch.x, dispatch.y, dispatch.z);
    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    std::vector<unsigned int> voxelData(gridSize.x * gridSize.y * gridSize.z);

    glBindTexture(GL_TEXTURE_3D, voxelTexture);
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RED_INTEGER, GL_UNSIGNED_INT, voxelData.data());
    std::cout << "Voxel (0, 0, 0): " << voxelData[0] << std::endl;

    const GLubyte* renderer = glGetString(GL_RENDERER);
    const GLubyte* version = glGetString(GL_VERSION);
    std::cout << "Renderer: " << renderer << std::endl;
    std::cout << "OpenGL Version: " << version << std::endl;

    for (size_t i = 0; i < voxelData.size(); ++i)
    {
        voxelGrid[i] = (voxelData[i] > 0);
    }
}

void ModelVoxelizer::voxelizeModel()
{
    ZoneScoped;

    printf("VOXELIZING\n");
    if (!loadedModel)
    {
        return;
    }

    setupBoundingBox();

    triangleVoxelization(voxelGrid);
    if (isExtensionSupported("GL_NV_conservative_raster"))
    {
        printf("Conservative rasterization supported\n");
    }
    else
    {
        printf("Conservative rasterization not supported\n");
    }

    generateVoxelMesh();
}

void ModelVoxelizer::generateVoxelMesh()
{
    ZoneScoped;

    printf("GENERATING VOXEL MESH\n");

    std::vector cubeVertices = {
        // Back
        VertexPositionUvNormal(glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        VertexPositionUvNormal(glm::vec3(0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        VertexPositionUvNormal(glm::vec3(0.5f, 0.5f, -0.5f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
        VertexPositionUvNormal(glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, -1.0f)),

        // Front
        VertexPositionUvNormal(glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        VertexPositionUvNormal(glm::vec3(0.5f, -0.5f, 0.5f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        VertexPositionUvNormal(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
        VertexPositionUvNormal(glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 0.0f, 1.0f)),

        // Left
        VertexPositionUvNormal(glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec2(1.0f, 1.0f), -glm::vec3(1.0f, 0.0f, 0.0f)),
        VertexPositionUvNormal(glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec2(0.0f, 1.0f), -glm::vec3(1.0f, 0.0f, 0.0f)),
        VertexPositionUvNormal(glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 0.0f), -glm::vec3(1.0f, 0.0f, 0.0f)),
        VertexPositionUvNormal(glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec2(1.0f, 0.0f), -glm::vec3(1.0f, 0.0f, 0.0f)),

        // Right
        VertexPositionUvNormal(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec2(0.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        VertexPositionUvNormal(glm::vec3(0.5f, 0.5f, -0.5f), glm::vec2(1.0f, 1.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        VertexPositionUvNormal(glm::vec3(0.5f, -0.5f, -0.5f), glm::vec2(1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),
        VertexPositionUvNormal(glm::vec3(0.5f, -0.5f, 0.5f), glm::vec2(0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f)),

        // Bottom
        VertexPositionUvNormal(glm::vec3(-0.5f, -0.5f, -0.5f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        VertexPositionUvNormal(glm::vec3(0.5f, -0.5f, -0.5f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        VertexPositionUvNormal(glm::vec3(0.5f, -0.5f, 0.5f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
        VertexPositionUvNormal(glm::vec3(-0.5f, -0.5f, 0.5f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),

        // Top
        VertexPositionUvNormal(glm::vec3(-0.5f, 0.5f, -0.5f), glm::vec2(0.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        VertexPositionUvNormal(glm::vec3(0.5f, 0.5f, -0.5f), glm::vec2(1.0f, 1.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        VertexPositionUvNormal(glm::vec3(0.5f, 0.5f, 0.5f), glm::vec2(1.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f)),
        VertexPositionUvNormal(glm::vec3(-0.5f, 0.5f, 0.5f), glm::vec2(0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0))
    };

    std::vector<GLuint> cubeIndices = {
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

    // Setup

    glGenVertexArrays(1, &voxelVAO);
    glGenBuffers(1, &voxelVBO);
    glGenBuffers(1, &voxelEBO);
    glGenBuffers(1, &instanceVBO);

    {
        glBindVertexArray(voxelVAO);

        // Cube VBO (vertices)
        glBindBuffer(GL_ARRAY_BUFFER, voxelVBO);
        glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(VertexPositionUvNormal), cubeVertices.data(), GL_STATIC_DRAW);

        // Cube EBO (indices)
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, voxelEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeIndices.size() * sizeof(GLuint), cubeIndices.data(), GL_STATIC_DRAW);

        // Position attribute (location = 0)
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPositionUvNormal), reinterpret_cast<void*>(offsetof(VertexPositionUvNormal, position)));

        // Texture Coordinate attribute (location = 2)
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(VertexPositionUvNormal), reinterpret_cast<void*>(offsetof(VertexPositionUvNormal, uv)));

        // Normal attribute (location = 3)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexPositionUvNormal), reinterpret_cast<void*>(offsetof(VertexPositionUvNormal, normal)));

        // Generate and bind instance VBO for voxel positions
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, activeVoxels.size() * sizeof(glm::vec3), activeVoxels.data(), GL_STATIC_DRAW);

        // Instance Position attribute (location = 1)
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), reinterpret_cast<void*>(0));
        glVertexAttribDivisor(1, 1); // Tell OpenGL this is per-instance data
    }
    glBindVertexArray(0);

    activeVoxels.clear();
    glm::vec3 gridCenter = minBounds + (glm::vec3(gridSize) * 0.5f);

    static std::random_device rd;

    // VoxelChunkData
    chunkData = std::make_shared<VoxelChunkData>(gridSize);

#pragma omp parallel for collapse(3) schedule(dynamic)
    for (int z = 0; z < gridSize.z; ++z)
    {
        for (int y = 0; y < gridSize.y; ++y)
        {
            for (int x = 0; x < gridSize.x; ++x)
            {
                static std::mt19937 gen(rd());
                std::uniform_int_distribution<uint16_t> dis(0, static_cast<uint16_t>(Constants::VoxelChunk::maxMaterialCount - 1));
                uint16_t randomizedIndex = dis(gen);
                chunkData->setVoxelOccupancy(glm::ivec3(x, y, z), voxelGrid[z * gridSize.x * gridSize.y + y * gridSize.x + x]);
                chunkData->setVoxelMaterial(glm::ivec3(x, y, z), MaterialManager::getInstance().getMaterialByIndex(randomizedIndex));

                if (voxelGrid[z * gridSize.x * gridSize.y + y * gridSize.x + x])
                {
                    glm::vec3 voxelWorldPosition = minBounds + glm::vec3(x, y, z);
                    glm::vec3 centeredVoxelPosition = voxelWorldPosition - gridCenter;
                    activeVoxels.emplace_back(centeredVoxelPosition);
                }
            }
        }
    }
    std::cout << "Active Voxels Count: " << activeVoxels.size() << std::endl;

    isVoxelized = true;
    std::cout << "VOXELIZED!" << std::endl;
}

void ModelVoxelizer::drawVoxels(const std::shared_ptr<ShaderProgram>& shader, glm::vec3 cameraPosition, glm::vec3 cameraForwardDirection, glm::vec3 cameraUpDirection, glm::ivec2 windowSize, float zoom)
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
    glm::mat4 view = glm::lookAt(cameraPosition, cameraPosition + cameraForwardDirection, cameraUpDirection);
    // glm::mat4 projection = glm::perspective(glm::radians(60.0f), static_cast<float>(windowSize.x) / static_cast<float>(windowSize.y), 0.001f, 1000.0f);
    glm::mat4 projection = glm::ortho(
        -zoom * static_cast<float>(windowSize.x) / windowSize.y, // Left
        zoom * static_cast<float>(windowSize.x) / windowSize.y, // Right
        -zoom, // Bottom
        zoom, // Top
        0.001f, 1000.0f // Near and far clipping planes
    );
    glm::mat4 model = glm::mat4(1.0f); // Initialize to identity matrix
    model = glm::scale(model, voxelSize); // Apply scaling

    glUniformMatrix4fv(glGetUniformLocation(shader->programId, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shader->programId, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shader->programId, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // Material settings for Phong Shader
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
    {
        glDrawElementsInstanced(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0, activeVoxels.size());
    }
    glBindVertexArray(0);
}

void ModelVoxelizer::addToWorld(glm::vec3 position, glm::quat rotation)
{
    // VoxelChunkComponent
    // Pass along scene object
    auto voxelChunkObject = sceneObject->createChildObject("Voxelized model");
    chunkComponent = voxelChunkObject->addComponent<VoxelChunkComponent>();

    allChunkComponents.push_back(chunkComponent);

    // Set Default Position and Rotation
    if (position == glm::vec3(0.0f))
    {
        position = cameraTransform->getGlobalPosition() + cameraTransform->getForwardDirection() * 50.0f;
    }
    if (rotation == glm::quat(1.0f, 0.0f, 0.0f, 0.0f))
    {
        rotation = glm::angleAxis(glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
    }

    chunkComponent->getTransform()->addGlobalPosition(position);
    chunkComponent->getTransform()->addGlobalRotation(rotation);

    // Send to World
    VoxelChunkCommandBuffer commandBuffer {};
    commandBuffer.copyFrom(chunkData);
    commandBuffer.setExistsOnGpu(true);

    VoxelChunkManager::getInstance().submitCommandBuffer(chunkComponent, commandBuffer);
}

void ModelVoxelizer::clearResources()
{
    // Reset shared pointers
    loadedModel.reset();
    chunkData.reset();

    for (auto& chunkComponent : allChunkComponents)
    {
        chunkComponent.reset();
    }

    // Clear vectors
    voxelGrid.clear();
    voxelMesh.clear();
    activeVoxels.clear();

    // Reset OpenGL buffers
    if (voxelVAO)
    {
        glDeleteVertexArrays(1, &voxelVAO);
        voxelVAO = 0;
    }
    if (voxelVBO)
    {
        glDeleteBuffers(1, &voxelVBO);
        voxelVBO = 0;
    }
    if (voxelEBO)
    {
        glDeleteBuffers(1, &voxelEBO);
        voxelEBO = 0;
    }
    if (instanceVBO)
    {
        glDeleteBuffers(1, &instanceVBO);
        instanceVBO = 0;
    }

    // Reset other variables

    gridSize = glm::ivec3(0);
    voxelSize = glm::vec3(0.0f);
    minBounds = glm::vec3(0.0f);
    maxBounds = glm::vec3(0.0f);
    isVoxelized = false;
}
