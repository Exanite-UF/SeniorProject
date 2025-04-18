#pragma once

#include <src/utilities/OpenGl.h>
#include <src/voxelizer/Model.h>
#include <src/world/VoxelChunkComponent.h>
#include <src/world/VoxelChunkData.h>

struct HashFunction
{
    template <typename T>
    std::size_t operator()(const glm::vec<3, T>& v) const
    {
        std::size_t h1 = std::hash<T> {}(v.x);
        std::size_t h2 = std::hash<T> {}(v.y);
        std::size_t h3 = std::hash<T> {}(v.z);

        return h1 ^ (h2 << 1) ^ (h3 << 2); // Combine the individual hashes
    }
};

class ModelVoxelizer
{
    struct VertexPositionUvNormal
    {
        glm::vec3 position {};
        glm::vec2 uv {};
        glm::vec3 normal {};

        explicit VertexPositionUvNormal(
            const glm::vec3& position,
            const glm::vec2& uv,
            const glm::vec3& normal)
        {
            this->position = position;
            this->uv = uv;
            this->normal = normal;
        }
    };

private:
    std::shared_ptr<Model> loadedModel {};
    int gridResolution {};
    glm::ivec3 gridSize {};
    glm::vec3 voxelSize {};
    std::vector<bool> voxelGrid {};
    glm::vec3 minBounds {};
    glm::vec3 maxBounds {};

    std::vector<Vertex> voxelMesh {};

    // Voxel Rendering
    unsigned int voxelVAO {};
    unsigned int voxelVBO {};
    unsigned int voxelEBO {};
    unsigned int instanceVBO {};
    std::vector<glm::vec3> activeVoxels {};
    std::shared_ptr<TransformComponent> cameraTransform {};
    std::vector<std::shared_ptr<VoxelChunkComponent>> allChunkComponents {};

    // Chunk Data
    std::shared_ptr<VoxelChunkData> chunkData {};
    std::shared_ptr<GameObject> sceneObject {};

    void setupBoundingBox();

    // Helpers
    void projectTriangle(const glm::vec3& axis,
        const glm::vec3 triVertices[3],
        float& min, float& max)
    {
        min = max = glm::dot(axis, triVertices[0]);
        for (int i = 1; i < 3; ++i)
        {
            float val = glm::dot(axis, triVertices[i]);
            min = std::min(min, val);
            max = std::max(max, val);
        }
    }

    void projectBox(const glm::vec3& axis, const glm::vec3& boxCenter,
        const glm::vec3& boxHalfSize, float& min, float& max)
    {
        float centerProj = glm::dot(axis, boxCenter);
        float extent = boxHalfSize.x * std::abs(axis.x) + boxHalfSize.y * std::abs(axis.y) + boxHalfSize.z * std::abs(axis.z);
        min = centerProj - extent;
        max = centerProj + extent;
    }

    glm::ivec3 worldToGrid(const glm::vec3& worldPosition)
    {
        glm::ivec3 gridPosition;
        gridPosition.x = static_cast<int>(std::floor((worldPosition.x - minBounds.x) / voxelSize.x));
        gridPosition.y = static_cast<int>(std::floor((worldPosition.y - minBounds.y) / voxelSize.y));
        gridPosition.z = static_cast<int>(std::floor((worldPosition.z - minBounds.z) / voxelSize.z));
        return gridPosition;
    }

    bool overlaps(float min1, float max1, float min2, float max2)
    {
        return max1 >= min2 && max2 >= min1;
    }

    bool triangleBoxOverlap(
        const glm::vec3& boxCenter,
        const glm::vec3& boxHalfSize,
        const Vertex triVertices[3]);

    bool triangleIntersection(
        const Triangle& tri,
        const glm::vec3& voxelMin);

    void triangleVoxelization(std::vector<bool>& voxels);

    void raymarchVoxelization(std::vector<bool>& voxels);

    void generateVoxelMesh();

public:
    bool isVoxelized = false;
    std::shared_ptr<VoxelChunkComponent> chunkComponent {};

    ~ModelVoxelizer();

    void loadModel(char* path);
    std::shared_ptr<Model> getModel();
    std::shared_ptr<VoxelChunkData> getChunkData();
    void voxelizeModel(int option = 0);
    void setSceneObject(std::shared_ptr<GameObject> sceneObject) { this->sceneObject = sceneObject; }
    void setSceneCameraTransform(std::shared_ptr<TransformComponent> cameraTransform) { this->cameraTransform = cameraTransform; }
    std::shared_ptr<VoxelChunkComponent> getChunkComponent() { return chunkComponent; }
    std::vector<std::shared_ptr<VoxelChunkComponent>> getChunkComponents() { return allChunkComponents; }
    
    void drawVoxels(const std::shared_ptr<ShaderProgram>& shader, glm::vec3 cameraPosition, glm::vec3 cameraForwardDirection, glm::vec3 cameraUpDirection, glm::ivec2 windowSize);

    void addToWorld(glm::vec3 position = glm::vec3(0.0f), glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f));

    void clearResources();
};
